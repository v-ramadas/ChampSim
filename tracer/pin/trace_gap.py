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

        "pr": [0x2680, 0x28a2], # Good
        "pr_spmv": [0x2660, 0x2804],
        "sssp": [0x2a10, 0x3377],
        "tc": [0x2350, 0x2410],
        "bfs": [0x2500, 0x27be],
        "bc": [0x2bd0, 0x343a],
        "cc": [0x2770, 0x2839],
        "cc_sv": [0x1720, 0x1970],
        }

binary = os.path.basename(args.b)
graph = os.path.basename(args.g)

if binary not in appDict.keys():
    print("The workload binary wasn't profiled earlier and we don't know where the ROI starts and ends")
    exit()

#traceFileName = "ubench_{}.drop_{}M.length_{}M.size_2MB.champsimtrace".format(
#        binary, int(args.s/1_000_000), int(args.t/1_000_000))
traceFileName = "gap_st_{}.{}.drop_{}M.length_{}M.champsimtrace".format(
        binary, graph, int(args.s/1_000_000), int(args.t/1_000_000))
runCommand = []
runCommand.append(os.path.join(os.getenv("PIN_ROOT"), "pin"))
runCommand.append("-t obj-intel64/champsim_tracer.so")
runCommand.append("-s " + str(args.s))
runCommand.append("-t " + str(args.t))
runCommand.append("-o " + os.path.join('traces', traceFileName))
runCommand.append("-roiStart " + hex(appDict[binary][0]))
runCommand.append("-roiEnd " + hex(appDict[binary][1]))
runCommand.append("-bmkName gapbs/" + binary)
runCommand.append("--")
runCommand.append(args.b)
runCommand.append("-f")
runCommand.append(args.g)
if binary == "tc":
        runCommand.append(" -s")
#runCommand.append("-i 10")

cmd = ' '.join(runCommand)
print("Running ", cmd)

os.system(cmd)
