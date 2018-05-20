#!/usr/bin/env python

import re
import os
import fileinput
import math
import pandas as pd
from pandas import *
from pylab import *
from datetime import *
from collections import OrderedDict
from IPython import embed
import matplotlib as mpl

TARGET_DIR = "/home/simon/simonduq.github.io/_runs"

pd.set_option('display.max_rows', 48)
pd.set_option('display.width', None)
pd.set_option('display.max_columns', None)

networkFormationTime = None
parents = {}

def calculateHops(node):
    hops = 0
    while(parents[node] != None):
        node = parents[node]
        hops += 1
    return hops

def calculateChildren(node):
    children = 0
    for n in parents.keys():
        if(parents[n] == node):
            children += 1
    return children

def updateTopology(child, parent):
    global parents
    if not child in parents:
        parents[child] = {}
    if not parent in parents:
        parents[parent] = None
    parents[child] = parent

def parseRPL(log):
    res = re.compile('.*? rank (\d*).*?dioint (\d*).*?nbr count (\d*)').match(log)
    if res:
        rank = int(res.group(1))
        trickle = (2**int(res.group(2)))/(60*1000.)
        nbrCount = int(res.group(3))
        return {'event': 'rank', 'rank': rank, 'trickle': trickle }
    res = re.compile('parent switch: .*? -> .*?-(\d*)$').match(log)
    if res:
        parent = int(res.group(1))
        return {'event': 'switch', 'pswitch': parent }
    res = re.compile('sending a (.+?) ').match(log)
    if res:
        message = res.group(1)
        return {'event': 'sending', 'message': message }
    res = re.compile('links: 6G-(\d+)\s*to 6G-(\d+)').match(log)
    if res:
        child = int(res.group(1))
        parent = int(res.group(2))
        updateTopology(child, parent)
        return None
    res = re.compile('links: end of list').match(log)
    if res:
        # This was the last line, commit full topology
        return {'event': 'topology' }
    return None

def parseEnergest(log):
    res = re.compile('Radio Tx\s*:\s*(\d*)/\s*(\d+)').match(log)
    if res:
        tx = float(res.group(1))
        total = float(res.group(2))
        return {'channel-utilization': 100.*tx/total }
    res = re.compile('Radio total\s*:\s*(\d*)/\s*(\d+)').match(log)
    if res:
        radio = float(res.group(1))
        total = float(res.group(2))
        return {'duty-cycle': 100.*radio/total }
    return None

def parseApp(log):
    res = re.compile('Sending (.+?) (\d+) to 6G-(\d+)').match(log)
    if res:
        type = res.group(1)
        id = int(res.group(2))
        dest = int(res.group(3))
        return {'event': 'send', 'type': type, 'id': id, 'node': dest }
    res = re.compile('Received (.+?) (\d+) from 6G-(\d+)').match(log)
    if res:
        type = res.group(1)
        id = int(res.group(2))
        src = int(res.group(3))
        return {'event': 'recv', 'type': type, 'id': id, 'src': src }
    return None

def parseLine(line):
    res = re.compile('\s*([.\d]+)\\tID:(\d+)\\t\[(.*?):(.*?)\](.*)$').match(line)
    if res:
        time = float(res.group(1))
        nodeid = int(res.group(2))
        level = res.group(3).strip()
        module = res.group(4).strip()
        log = res.group(5).strip()
        return time, nodeid, level, module, log
    return None, None, None, None, None

def doParse(dir):
    global networkFormationTime

    file = os.path.join(dir, "logs", "log.txt")

    time = None
    lastPrintedTime = 0

    arrays = {
        "packets": [],
        "energest": [],
        "ranks": [],
        "trickle": [],
        "switches": [],
        "topology": [],
    }

    print("\nProcessing %s" %(file))
    for line in open(file, 'r').readlines():
        # match time, id, module, log; The common format for all log lines
        time, nodeid, level, module, log = parseLine(line)

        if time == None:
            # malformed line
            continue

        if time - lastPrintedTime >= 60:
            print("%u, "%(time / 60),end='', flush=True)
            lastPrintedTime = time

        entry = {
            "timestamp": timedelta(seconds=time),
            "node": nodeid,
        }

        if module == "App":
            ret = parseApp(log)
            if(ret != None):
                entry.update(ret)
                if(ret['event'] == 'send' and ret['type'] == 'request'):
                    # populate series of sent requests
                    entry['pdr'] = 0.
                    arrays["packets"].append(entry)
                    if networkFormationTime == None:
                        networkFormationTime = time
                elif(ret['event'] == 'recv' and ret['type'] == 'response'):
                    # update sent request series with latency and PDR
                    txElement = [x for x in arrays["packets"] if x['event']=='send' and x['id']==ret['id']][0]
                    txElement['latency'] = time - txElement['timestamp'].seconds
                    txElement['pdr'] = 100.

        if module == "Energest":
            ret = parseEnergest(log)
            if(ret != None):
                entry.update(ret)
                arrays["energest"].append(entry)

        if module == "RPL":
            ret = parseRPL(log)
            if(ret != None):
                entry.update(ret)
                if(ret['event'] == 'rank'):
                    arrays["ranks"].append(entry)
                    arrays["trickle"].append(entry)
                elif(ret['event'] == 'switch'):
                    arrays["switches"].append(entry)
                elif(ret['event'] == 'sending'):
                    if not ret['message'] in arrays:
                        arrays[ret['message']] = []
                    arrays[ret['message']].append(entry)
                elif(ret['event'] == 'topology'):
                    for n in parents.keys():
                        nodeEntry = entry.copy()
                        nodeEntry["node"] = n
                        nodeEntry["hops"] = calculateHops(n)
                        nodeEntry["children"] = calculateChildren(n)
                        arrays["topology"].append(nodeEntry)

    print("")

    # Remove last few packets -- might be in-flight when test stopped
    arrays["packets"] = arrays["packets"][0:-10]

    dfs = {}
    for key in arrays.keys():
        if(len(arrays[key]) > 0):
            df = DataFrame(arrays[key])
            dfs[key] = df.set_index("timestamp")

    return dfs

def outputStats(file, df, metric, agg, name, metricLabel = None):
    perNode = getattr(df.groupby("node")[metric], agg)()
    perTime = getattr(df.groupby([pd.Grouper(freq="2Min")])[metric], agg)()

    file.write("  %s:\n" %(metricLabel if metricLabel != None else metric))
    file.write("    name: %s\n" %(name))
    file.write("    per-node:\n")
    file.write("      x: [%s]\n" %(", ".join(["%u"%x for x in sort(df.node.unique())])))
    file.write("      y: [%s]\n" %(', '.join(["%.4f"%(x) for x in perNode])))
    file.write("    per-time:\n")
    file.write("      x: [%s]\n" %(", ".join(["%u"%x for x in range(0, 2*len(df.groupby([pd.Grouper(freq="2Min")]).mean().index), 2)])))
    file.write("      y: [%s]\n" %(', '.join(["%.4f"%(x) for x in perTime]).replace("nan", "null")))

def main():
    if len(sys.argv) < 1:
        return
    else:
        dir = sys.argv[1].rstrip('/')
    date = open(os.path.join(dir, ".started"), 'r').readlines()[0].strip()
    duration = open(os.path.join(dir, "duration"), 'r').readlines()[0].strip()

    setup = "test-tsch-optims"
    repository = "simonduq/contiki-ng"
    branch = "wip/testbed"
    path = "examples/benchmarks/rpl-req-resp"
    configuration = "CONFIG_TSCH_OPTIMS"

    jobId = dir.split("_")[0]
    commit = "ae26163dd07b6ebf3a2d0aa6eeec52dd2f0b5768"

    # Parse the original log
    dfs = doParse(dir)

    if len(dfs) == 0:
        return

    outFile = open(os.path.join(TARGET_DIR, "%s.md"%(jobId)), "w")
    outFile.write("---\n")
    outFile.write("date: %s\n" %(date))
    outFile.write("duration: %s\n" %(duration))

    outFile.write("setup: %s\n" %(setup))
    #outFile.write("repository: %s\n" %(repository))
    #outFile.write("branch: %s\n" %(branch))
    #outFile.write("path: %s\n" %(branch))
    #outFile.write("configuration: %s\n" %(configuration))

    outFile.write("commit: %s\n" %(commit))
    outFile.write("global-stats:\n")
    outFile.write("  pdr: %.4f\n" %(dfs["packets"]["pdr"].mean()))
    outFile.write("  loss-rate: %.e\n" %(1-(dfs["packets"]["pdr"].mean()/100)))
    outFile.write("  packets-sent: %u\n" %(dfs["packets"]["pdr"].count()))
    outFile.write("  packets-received: %u\n" %(dfs["packets"]["pdr"].sum()/100))
    outFile.write("  latency: %.4f\n" %(dfs["packets"]["latency"].mean()))
    outFile.write("  duty-cycle: %.2f\n" %(dfs["energest"]["duty-cycle"].mean()))
    outFile.write("  channel-utilization: %.2f\n" %(dfs["energest"]["channel-utilization"].mean()))
    outFile.write("  network-formation-time: %.2f\n" %(networkFormationTime))
    outFile.write("stats:\n")

    # Output relevant metrics
    outputStats(outFile, dfs["packets"], "pdr", "mean", "End-to-end PDR (%)")
    outputStats(outFile, dfs["packets"], "latency", "mean", "Round-trip latency (s)")

    outputStats(outFile, dfs["energest"], "duty-cycle", "mean", "Radio duty cycle (%)")
    outputStats(outFile, dfs["energest"], "channel-utilization", "mean", "Channel utilization (%)")

    outputStats(outFile, dfs["ranks"], "rank", "mean", "RPL rank (ETX-128)")
    outputStats(outFile, dfs["switches"], "pswitch", "count", "RPL parent switches (#)")
    outputStats(outFile, dfs["trickle"], "trickle", "mean", "RPL Trickle period (min)")

    outputStats(outFile, dfs["DIS"], "message", "count", "RPL DIS sent (#)", "rpl-dis")
    outputStats(outFile, dfs["unicast-DIO"], "message", "count", "RPL uDIO sent (#)", "rpl-udio")
    outputStats(outFile, dfs["multicast-DIO"], "message", "count", "RPL mDIO sent (#)", "rpl-mdio")
    outputStats(outFile, dfs["DAO"], "message", "count", "RPL DAO sent (#)", "rpl-dao")
    outputStats(outFile, dfs["DAO-ACK"], "message", "count", "RPL DAO-ACK sent (#)", "rpl-daoack")

    outputStats(outFile, dfs["topology"], "hops", "mean", "RPL hop count (#)")
    outputStats(outFile, dfs["topology"], "children", "mean", "RPL children count (#)")

    outFile.write("---\n")
    outFile.write("\n{% include run.md %}\n")

main()
