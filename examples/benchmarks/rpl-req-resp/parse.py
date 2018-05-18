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

def parseRPL(log):
    res = re.compile('.*? rank (\d*).*?nbr count (\d*)').match(log)
    if res:
        rank = int(res.group(1))
        nbrCount = int(res.group(2))
        return {'event': 'rank', 'rank': rank }
    res = re.compile('parent switch: .*? -> .*?-(\d*)$').match(log)
    if res:
        parent = int(res.group(1))
        return {'event': 'switch', 'parent': parent }
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
        return {'event': 'send', 'type': type, 'id': id, 'dest': dest }
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

        if module == "App":
            ret = parseApp(log)
            if(ret != None):
                if(ret['event'] == 'send' and ret['type'] == 'request'):
                    # populate series of sent requests
                    ret["src"] = nodeid
                    ret["timestamp"] = timedelta(seconds=time)
                    ret['received'] = False
                    arrays["packets"].append(ret)
                elif(ret['event'] == 'recv' and ret['type'] == 'response'):
                    # update sent request series with latency and received flag
                    txElement = [x for x in arrays["packets"] if x['event']=='send' and x['id']==ret['id']][0]
                    txElement['latency'] = time - txElement['timestamp'].seconds
                    txElement['received'] = True

        if module == "Energest":
            ret = parseEnergest(log)
            if(ret != None):
                ret["node"] = nodeid
                ret["timestamp"] = timedelta(seconds=time)
                arrays["energest"].append(ret)

        if module == "RPL":
            ret = parseRPL(log)
            if(ret != None):
                if(ret['event'] == 'rank'):
                    ret["node"] = nodeid
                    ret["timestamp"] = timedelta(seconds=time)
                    arrays["ranks"].append(ret)
                elif(ret['event'] == 'switch'):
                    ret["node"] = nodeid
                    ret["timestamp"] = timedelta(seconds=time)
                    arrays["switches"].append(ret)

    print("")

    dfs = {}
    for key in arrays.keys():
        if(len(arrays[key]) > 0):
            df = DataFrame(arrays[key])
            dfs[key] = df.set_index("timestamp")

    return dfs

def main():
    if len(sys.argv) < 1:
        return
    else:
        dir = sys.argv[1].rstrip('/')
    file = os.path.join(dir, "logs", "log.txt")

    # Parse the original log
    dfs = doParse(dir)

    ###########################################################
    # PDR
    ###########################################################
    print("Latency, for each packet: ")
    print(dfs["packets"]['latency'].as_matrix())

    perNode = dfs["packets"].groupby("dest").agg({'received': {'received': 'sum', 'sent': 'count'}})
    perNode.columns = perNode.columns.droplevel(0)
    perNode["pdr"] =  100 * perNode["received"] / perNode["sent"]

    print("PDR, for each node: ")
    print(perNode['pdr'].as_matrix())

    perTime = dfs["packets"].groupby([pd.Grouper(freq="2Min")]).agg({'received': {'received': 'sum', 'sent': 'count'}})
    perTime.columns = perTime.columns.droplevel(0)
    perTime["pdr"] =  100 * perTime["received"] / perTime["sent"]

    print("PDR, timeline: ")
    print(perTime['pdr'].as_matrix())

    ###########################################################
    # Latency
    ###########################################################
    print("Latency, for each packet: ")
    print(dfs["packets"]['latency'].as_matrix())

    perNode = dfs["packets"][["dest", "latency"]].groupby("dest").mean()
    print("Latency, for each node: ")
    print(perNode['latency'].as_matrix())

    perTime = dfs["packets"][["dest", "latency"]].groupby([pd.Grouper(freq="2Min")]).mean()
    print("Latency, timeline: ")
    print(perTime['latency'].as_matrix())

    ###########################################################
    # Energest
    ###########################################################

    perNode = dfs["energest"].groupby("node").mean()
    print("Duty Cycle, for each node: ")
    print(perNode['duty-cycle'].as_matrix())

    print("Channel Utilization, for each node: ")
    print(perNode['channel-utilization'].as_matrix())

    perTime = dfs["energest"].groupby([pd.Grouper(freq="2Min")]).mean()
    print("Duty Cycle, timeline: ")
    print(perTime['duty-cycle'].as_matrix())

    print("Channel Utilization, timeline: ")
    print(perTime['channel-utilization'].as_matrix())

    ###########################################################
    # Ranks
    ###########################################################
    perNode = dfs["ranks"].groupby("node").mean()
    print("Ranks, for each node: ")
    print(perNode['rank'].as_matrix())
    # all info for a given nbr: dfs["ranks"][(dfs["ranks"].node==1)]["neighbors"].as_matrix()

    perTime = dfs["ranks"].groupby([pd.Grouper(freq="2Min")]).mean()
    print("Ranks, timeline: ")
    print(perTime['rank'].as_matrix())

    ###########################################################
    # Parent switches
    ###########################################################
    perNode = dfs["switches"].groupby("node").count()
    print("Ranks, for each node: ")
    print(perNode['parent'].as_matrix())
    # all info for a given nbr: dfs["ranks"][(dfs["ranks"].node==1)]["neighbors"].as_matrix()

    perTime = dfs["switches"].groupby([pd.Grouper(freq="2Min")]).count()
    print("Ranks, timeline: ")
    print(perTime['parent'].as_matrix())

main()
