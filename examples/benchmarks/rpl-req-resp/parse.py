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

pd.set_option('display.max_rows', 48)
pd.set_option('display.width', None)
pd.set_option('display.max_columns', None)

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
    res = re.compile('.*? rank (\d*).*?nbr count (\d*)').match(log)
    if res:
        rank = int(res.group(1))
        nbrCount = int(res.group(2))
        return {'event': 'rank', 'rank': rank }
    res = re.compile('parent switch: .*? -> .*?-(\d*)$').match(log)
    if res:
        parent = int(res.group(1))
        return {'event': 'switch', 'pswitch': parent }
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
    file = os.path.join(dir, "logs", "log.txt")

    time = None
    lastPrintedTime = 0

    arrays = {
        "packets": [],
        "energest": [],
        "ranks": [],
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
                elif(ret['event'] == 'switch'):
                    arrays["switches"].append(entry)
                elif(ret['event'] == 'topology'):
                    for n in parents.keys():
                        nodeEntry = entry.copy()
                        nodeEntry["node"] = n
                        nodeEntry["hops"] = calculateHops(n)
                        nodeEntry["children"] = calculateChildren(n)
                        arrays["topology"].append(nodeEntry)

    print("")

    dfs = {}
    for key in arrays.keys():
        if(len(arrays[key]) > 0):
            df = DataFrame(arrays[key])
            dfs[key] = df.set_index("timestamp")

    return dfs

def outputStats(df, metric, agg):
    perNode = getattr(df[["node", metric]].groupby("node"), agg)()
    print("%s, for each node: " %(metric))
    print(perNode[metric].as_matrix())

    perTime = getattr(df[["node", metric]].groupby([pd.Grouper(freq="2Min")]), agg)()
    print("%s, timeline: " %(metric))
    print(perTime[metric].as_matrix())

def main():
    if len(sys.argv) < 1:
        return
    else:
        dir = sys.argv[1].rstrip('/')
    file = os.path.join(dir, "logs", "log.txt")

    # Parse the original log
    dfs = doParse(dir)

    # Output relevant metrics
    outputStats(dfs["packets"], "pdr", "mean")
    outputStats(dfs["packets"], "latency", "mean")

    outputStats(dfs["energest"], "duty-cycle", "mean")
    outputStats(dfs["energest"], "channel-utilization", "mean")

    outputStats(dfs["ranks"], "rank", "mean")

    outputStats(dfs["switches"], "pswitch", "count")

    outputStats(dfs["topology"], "hops", "mean")
    outputStats(dfs["topology"], "children", "mean")

main()