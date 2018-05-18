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
    res = re.compile('.*?nbr count (\d*)').match(log)
    if res:
        nbrCount = int(res.group(1))
        return {'event': 'neighbors', 'neighbors': nbrCount }
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

    packets = []
    energest = []
    neighbors = []

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
                    packets.append(ret)
                elif(ret['event'] == 'recv' and ret['type'] == 'response'):
                    # update sent request series with latency and received flag
                    txElement = [x for x in packets if x['event']=='send' and x['id']==ret['id']][0]
                    txElement['latency'] = time - txElement['timestamp'].seconds
                    txElement['received'] = True

        if module == "Energest":
            ret = parseEnergest(log)
            if(ret != None):
                ret["node"] = nodeid
                ret["timestamp"] = timedelta(seconds=time)
                energest.append(ret)

        if module == "RPL":
            ret = parseRPL(log)
            if(ret != None):
                if(ret['event'] == 'neighbors'):
                    ret["node"] = nodeid
                    ret["timestamp"] = timedelta(seconds=time)
                    neighbors.append(ret)

    print("")

    dfPackets = DataFrame(packets)
    dfPackets = dfPackets.set_index("timestamp")
    dfEnergest = DataFrame(energest)
    dfEnergest = dfEnergest.set_index("timestamp")
    dfNeighbors = DataFrame(neighbors)
    dfNeighbors = dfNeighbors.set_index("timestamp")

    return dfPackets, dfEnergest, dfNeighbors

def main():
    if len(sys.argv) < 1:
        return
    else:
        dir = sys.argv[1].rstrip('/')
    file = os.path.join(dir, "logs", "log.txt")

    # Parse the original log
    dfPackets, dfEnergest, dfNeighbors = doParse(dir)

    ###########################################################
    # PDR
    ###########################################################
    print("Latency, for each packet: ")
    print(dfPackets['latency'].as_matrix())

    perNode = dfPackets.groupby("dest").agg({'received': {'received': 'sum', 'sent': 'count'}})
    perNode.columns = perNode.columns.droplevel(0)
    perNode["pdr"] =  100 * perNode["received"] / perNode["sent"]

    print("PDR, for each node: ")
    print(perNode['pdr'].as_matrix())

    perTime = dfPackets.groupby([pd.Grouper(freq="2Min")]).agg({'received': {'received': 'sum', 'sent': 'count'}})
    perTime.columns = perTime.columns.droplevel(0)
    perTime["pdr"] =  100 * perTime["received"] / perTime["sent"]

    print("PDR, timeline: ")
    print(perTime['pdr'].as_matrix())

    ###########################################################
    # Latency
    ###########################################################
    print("Latency, for each packet: ")
    print(dfPackets['latency'].as_matrix())

    perNode = dfPackets[["dest", "latency"]].groupby("dest").mean()
    print("Latency, for each node: ")
    print(perNode['latency'].as_matrix())

    perTime = dfPackets[["dest", "latency"]].groupby([pd.Grouper(freq="2Min")]).mean()
    print("Latency, timeline: ")
    print(perTime['latency'].as_matrix())

    ###########################################################
    # Energest
    ###########################################################

    perNode = dfEnergest.groupby("node").mean()
    print("Duty Cycle, for each node: ")
    print(perNode['duty-cycle'].as_matrix())

    print("Channel Utilization, for each node: ")
    print(perNode['channel-utilization'].as_matrix())

    perTime = dfEnergest.groupby([pd.Grouper(freq="2Min")]).mean()
    print("Duty Cycle, timeline: ")
    print(perTime['duty-cycle'].as_matrix())

    print("Channel Utilization, timeline: ")
    print(perTime['channel-utilization'].as_matrix())

    ###########################################################
    # Neighbors
    ###########################################################

    perNode = dfNeighbors.groupby("node").mean()
    print("Neihbors, for each node: ")
    print(perNode['neighbors'].as_matrix())
    # all info for a given nbr: dfNeighbors[(dfNeighbors.node==1)]["neighbors"].as_matrix()

    perTime = dfNeighbors.groupby([pd.Grouper(freq="2Min")]).mean()
    print("Neighbors, timeline: ")
    print(perTime['neighbors'].as_matrix())

    embed()

main()
