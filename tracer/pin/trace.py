#!/usr/bin/python3

import os
import argparse

parser = argparse.ArgumentParser()

parser.add_argument("-b", help="Path to binary file", default="", type=str)
parser.add_argument("-g", help="Path to input graph file", default="", type=str)
parser.add_argument("-s", help="Number of instructions to skip", default=0, type=int)
parser.add_argument("-t", help="Number of instructions to capture", default=100_000_000, type=int)

args = parser.parse_args()

appDict = {
#        "BC": [0x1bde3, 0x1c9cb], # Good
#        "BellmanFord": [0x15b4d, 0x15edb], # Bad input file. Needs weighted adjancency matrix
#        "BFS": [0x6c10, 0xb2fe], # Good
#        "BFS-Bitvector": [0x11f5f, 0x126df], # Good
#        "BFSCC": [0x87ea, 0x8eed], # Good
#        "CF": [0xf03e, 0xf85f], # Good. Needs weighted adjacency matrix
#        "Components": [0x8793, 0x8e3e], # Good
#        "Components-Shortcut": [0x8aa3, 0x90ab], # Good
#        "MIS": [0xeb08, 0xf07a], # Good
        "PageRank": [0x9586, 0x958a], # Good
#        "PageRankDelta": [0x12199, 0x127af], # Good
#        "Radii": [0x9454, 0x9b4e], # Good
#        "Triangle": [0x80c7, 0x84eb], # Good
#         "stream":[0x1a1,0x318],
#         "stride_2":[0x24e,0x288],
#         "stride_4":[0x24e, 0x288],
#         "stride_8":[0x24e, 0x288],
        }

binary = os.path.basename(args.b)
graph = os.path.basename(args.g)

if binary not in appDict.keys():
    print("The workload binary wasn't profiled earlier and we don't know where the ROI starts and ends")
    exit()

#traceFileName = "ubench_{}.drop_{}M.length_{}M.size_2MB.champsimtrace".format(
#        binary, int(args.s/1_000_000), int(args.t/1_000_000))
traceFileName = "ligra_{}.{}.drop_{}M.length_{}M.champsimtrace".format(
        binary, graph, int(args.s/1_000_000), int(args.t/1_000_000))
runCommand = []
runCommand.append(os.path.join(os.getenv("PIN_ROOT"), "pin"))
runCommand.append("-t obj-intel64/champsim_tracer.so")
runCommand.append("-s " + str(args.s))
runCommand.append("-t " + str(args.t))
runCommand.append("-o " + os.path.join('traces', traceFileName))
runCommand.append("-roiStart " + hex(appDict[binary][0]))
runCommand.append("-roiEnd " + hex(appDict[binary][1]))
runCommand.append("-bmkName " + binary)
runCommand.append("--")
runCommand.append(args.b)
runCommand.append(args.g)

cmd = ' '.join(runCommand)
print("Running ", cmd)

os.system(cmd)
