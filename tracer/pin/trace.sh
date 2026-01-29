#!/usr/bin/bash

#./trace.py -b ../../../ligra/apps/BC -g ../../../ligra/inputs/${1}.adj -s 100000000 -t 200000000
#./trace.py -b ../../../ligra/apps/BFS-Bitvector -g ../../../ligra/inputs/${1}.adj -s 0 -t 200000000
#./trace.py -b ../../../ligra/apps/BFSCC -g ../../../ligra/inputs/${1}.adj -s 100000000 -t 200000000
#./trace.py -b ../../../ligra/apps/Components -g ../../../ligra/inputs/${1}.adj -s 100000000 -t 200000000
#./trace.py -b ../../../ligra/apps/Components-Shortcut -g ../../../ligra/inputs/${1}.adj -s 100000000 -t 200000000
#./trace.py -b ../../../ligra/apps/MIS -g ../../../ligra/inputs/${1}.adj -s 100000000 -t 200000000
#./trace.py -b ../../../ligra/apps/PageRank -g ../../../ligra/inputs/ligra_graphs/${1} -s 0 -t 1000000000
#./trace.py -b ../../../ligra/apps/PageRankDelta -g ../../../ligra/inputs/${1}.adj -s 100000000 -t 200000000
#./trace.py -b ../../../ligra/apps/Radii -g ../../../ligra/inputs/${1}.adj -s 0 -t 200000000
#./trace.py -b ../../../ligra/apps/Triangle -g ../../../ligra/inputs/${1}.adj -s 100000000 -t 200000000


#./trace.py -b ../../../ligra/apps/BellmanFord -g ../../../ligra/inputs/${1}.weighted.adj -s 0 -t 200000000
#./trace.py -b ../../../ligra/apps/CF -g ../../../ligra/inputs/${1}.weighted.adj -s 100000000 -t 200000000

#./trace.py -b ../../../ubenchmarks/custom/stream  -s 0 -t 10000000
#./trace.py -b ../../../ubenchmarks/custom/stride_2  -s 0 -t 100000000
#./trace.py -b ../../../ubenchmarks/custom/stride_4  -s 0 -t 100000000
#./trace.py -b ../../../ubenchmarks/custom/stride_8  -s 0 -t 100000000
#./trace_gap.py -b ../../../gapbs/pr -g ../../../gapbs/inputs/com-LiveJournal/com-LiveJournal.mtx -s 0 -t 1000000000
#./trace_gap.py -b ../../../gapbs/bfs -g ../../../gapbs/inputs/com-LiveJournal/com-LiveJournal.mtx -s 0 -t 1000000000
#./trace_gap.py -b ../../../gapbs/sssp -g ../../../gapbs/inputs/com-LiveJournal/com-LiveJournal.mtx -s 0 -t 1000000000
#./trace_gap.py -b ../../../gapbs/bc -g ../../../gapbs/inputs/com-LiveJournal/com-LiveJournal.mtx -s 0 -t 1000000000
#./trace_gap.py -b ../../../gapbs/tc -g ../../../gapbs/inputs/com-LiveJournal/com-LiveJournal.mtx -s 0 -t 1000000000
#./trace_gap.py -b ../../../gapbs/cc -g ../../../gapbs/inputs/com-LiveJournal/com-LiveJournal.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/tc -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/bc -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/bfs -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/cc -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/cc_sv -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/pr -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/pr_spmv -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
./trace_gap.py -b ../../../gapbs/sssp -g ../../../gapbs/inputs/GAP-road/GAP-road.mtx -s 0 -t 1000000000
