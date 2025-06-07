import os
import argparse
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
import re

parser = argparse.ArgumentParser()

parser.add_argument("-d",
        type=str,
        default="",
        help="directory with run logs"
        )

args = parser.parse_args()


caches = ["L1I", "L1D", "LLC"]
block_size = [4, 8]

results_dir = os.path.join('results', args.d)

class Stats:
    class CacheStats:
        def __init__(self):
            self.hit_rate = 0
            self.total_hit_rate = 0
            self.unrealised_hit_rate = 0
            self.total_unrealised_hit_rate = 0
            self.hits = 0
            self.misses = 0
            self.accesses = 0
            self.total_misses = 0
            self.total_hits = 0
            self.total_accesses = 0
            self.unrealised_hits = 0
            self.total_unrealised_hits = 0
            self.compulsory_misses = 0
            self.capacity_misses = 0
            self.conflict_misses = 0
            self.cache_line_utilized = 0
            self.total_cache_line_utilized = 0
            self.evictions = 0
            self.total_evictions = 0
            return
        
    def __init__(self, cacheList):
        self.caches = {}
        for cache in cacheList:
            self.caches[cache] = {4: self.CacheStats(), 8: self.CacheStats()}
        return
    
    def setStats(self, line: str, block_size: int):
        line_list = line.split()
        if len(line_list) < 2:
            # Empty line. Skip it
            return

        match = re.match(r"cpu0->(?:cpu0_)?(.+)", line_list[0])
        if not match:
            return
        cache = match.group(1)
        if cache not in self.caches.keys():
            return
        
        if "TOTAL" != line_list[1]:
            # Not aggregate stats. Skip it
            return

        if "TOTAL_ACCESS:" == line_list[2]:
            self.caches[cache][block_size].total_accesses = int(line_list[3])
            self.caches[cache][block_size].total_hits = int(line_list[5])
            self.caches[cache][block_size].total_misses = int(line_list[7])
            self.caches[cache][block_size].unrealised_hits = int(line_list[9])
            self.caches[cache][block_size].total_hit_rate = float(self.caches[cache][block_size].total_hits/self.caches[cache][block_size].total_accesses)
            self.caches[cache][block_size].unrealised_hit_rate = float(line_list[13])
            self.caches[cache][block_size].total_unrealised_hits = int(line_list[11])
            self.caches[cache][block_size].total_unrealised_hit_rate = float(line_list[15])
        elif "EVICTIONS:" == line_list[2]:
            self.caches[cache][block_size].evictions = int(line_list[3])+1
            self.caches[cache][block_size].total_evictions = int(line_list[5])+1
            self.caches[cache][block_size].cache_line_utilization = 1 - (float(int(line_list[7])/self.caches[cache][block_size].evictions)*(block_size/64))
            self.caches[cache][block_size].total_cache_line_utilization = 1 - (float(int(line_list[9])/self.caches[cache][block_size].total_evictions)*(block_size/64))
        elif "ACCESS:" == line_list[2]:
            self.caches[cache][block_size].accesses = int(line_list[3])
            self.caches[cache][block_size].hits = int(line_list[5])
            self.caches[cache][block_size].misses = int(line_list[7])
            self.caches[cache][block_size].compulsory_misses = int(line_list[9])
            self.caches[cache][block_size].capacity_misses = int(line_list[11])
            self.caches[cache][block_size].conflict_misses = int(line_list[13])
        return

    def sort_data(benchmarks, yvalues, block_sizes):
        ligra_benchmarks = []
        ligra_yvalues = []
        ligra_block_sizes = []
        spec_benchmarks = []
        spec_yvalues = []
        spec_block_sizes = []
        
        for i in range(len(benchmarks)):
            expt_file = benchmarks[i]
            if "ligra" in expt_file:
                pattern = r"^ligra_ligra_(.*?)\."
                ligra_benchmarks.append(re.match(pattern, expt_file).group(1))
                ligra_yvalues.append(yvalues[i])
                ligra_block_sizes.append(block_sizes[i])
            else:
                pattern = r"^speccpu_\d+\.(\w+)-.*\.champsimtrace\.xz\.stdout$"
                spec_benchmarks.append(re.match(pattern, expt_file).group(1))
                spec_yvalues.append(yvalues[i])
                spec_block_sizes.append(block_sizes[i])
        data = {
            'benchmark': spec_benchmarks + ligra_benchmarks,
            'yvalue': spec_yvalues + ligra_yvalues,
            'block_size': spec_block_sizes + ligra_block_sizes
        }
        return data
     
    def barplot(title, xlabel, ylabel, min_value, max_value, benchmarks, yvalues, block_sizes):
        data = Stats.sort_data(benchmarks, yvalues, block_sizes)
        df = pd.DataFrame(data)
        plt.figure()
        sns.barplot(data=df, x='benchmark', y='yvalue', hue='block_size', palette='Set1')
        plt.title(title)
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.legend()
        plt.xticks(rotation=90)
        plt.ylim(min_value, max_value)
        plt.tight_layout()
        return

    def plot_mpki(stats, cache, block_sizes):
        num_instructions = 500_000_000
        kilo_instructions = 1_000
        benchmarks = []
        yvalues = []
        sizes = []

        for expt in stats.keys():
           # Store the base MPKI
            benchmarks.append(expt)
            mpki = float(stats[expt].caches[cache][4].misses/num_instructions)
            yvalues.append(mpki * kilo_instructions)
            sizes.append(64)    

        for block_size in block_sizes:
            for expt in stats.keys():
                benchmarks.append(expt)
                cache_stats = stats[expt].caches[cache][block_size]
                mpki = float((cache_stats.accesses - cache_stats.hits - cache_stats.unrealised_hits)/num_instructions)
                yvalues.append(mpki * kilo_instructions)
                sizes.append(block_size)    
        Stats.barplot(cache + '_MPKI', "Benchmarks", "MPKI", 0, kilo_instructions/10, benchmarks, yvalues, sizes)
        return
    
    def plot_utilization(stats, cache, block_sizes):
        benchmarks = []
        yvalues = []
        sizes = []
        for block_size in block_sizes:
            for expt in stats.keys():
                benchmarks.append(expt)
                util = stats[expt].caches[cache][block_size].total_cache_line_utilization
                yvalues.append(util*100)   
                sizes.append(block_size)
        Stats.barplot(cache + '_Utilization', "Benchmarks", "Cache Line Utilization (%)", 0, 100, benchmarks, yvalues, sizes)
        return
    
    def plot_hitrate(stats, cache, block_sizes):
        benchmarks = []
        yvalues = []
        sizes = []

        for expt in stats.keys():
            benchmarks.append(expt)
            hit_rate = float(stats[expt].caches[cache][4].hits/stats[expt].caches[cache][4].accesses)
            yvalues.append(hit_rate)
            sizes.append(64)

        for block_size in block_sizes:
            for expt in stats.keys():
                benchmarks.append(expt)
                hit_rate = float(stats[expt].caches[cache][block_size].unrealised_hit_rate)
                yvalues.append(hit_rate)
                sizes.append(block_size)
        Stats.barplot(cache + '_Hit_Rate', "Benchmarks", "Hit Rate", 0, 1.0, benchmarks, yvalues, sizes)
        return
    
    def plot_miss_breakdown(stats, cache, block_sizes):
        benchmarks = [""] * len(stats.keys())
        compulsory_misses = [0] * len(stats.keys())
        capacity_misses = [0] * len(stats.keys())
        conflict_misses = [0] * len(stats.keys())
        block_size = 4
        spec_idx = 0
        ligra_idx = int(len(stats.keys())/2)
        for expt in stats.keys():
            compulsory_miss = stats[expt].caches[cache][block_size].compulsory_misses
            capacity_miss = stats[expt].caches[cache][block_size].capacity_misses
            conflict_miss = stats[expt].caches[cache][block_size].conflict_misses
            total_misses = stats[expt].caches[cache][block_size].total_misses
            if "ligra" in expt:
                pattern = r"^ligra_ligra_(.*?)\."
                idx = ligra_idx
                ligra_idx +=1
            else:
                pattern = r"^speccpu_\d+\.(\w+)-.*\.champsimtrace\.xz\.stdout$"
                idx = spec_idx
                spec_idx += 1
            print (expt, idx, pattern)
            benchmarks[idx] = re.match(pattern, expt).group(1)
            compulsory_misses[idx] = float(compulsory_miss/total_misses)
            capacity_misses[idx] = float(capacity_miss/total_misses)
            conflict_misses[idx] = float(conflict_miss/total_misses)
        
        data = {
            'benchmark': benchmarks,
            'compulsory': compulsory_misses,
            'capacity': capacity_misses,
            'conflict': conflict_misses 
        }
        df = pd.DataFrame(data)
        sns.set(style='white')

        df.set_index('benchmark').plot(kind='bar', stacked=True, color=['steelblue', 'red', 'green'])
        plt.title(cache + "_Miss_Breakdown")
        plt.xlabel("Benchmarks")
        plt.ylabel("Fraction")
        plt.legend()
        plt.tight_layout()
        return

basePath = os.getcwd()

results = {}

for size in block_size:
    expt_dir = os.path.join(results_dir, "expt_" + str(size) + "b")
    os.chdir(os.path.join(basePath, expt_dir, 'outputs'))
    for expt_file in os.listdir(os.getcwd()):
        f = open(os.path.abspath(expt_file), 'r')
        if expt_file not in results.keys():
            results[expt_file] = Stats(caches)
        for line in f:
            if (len(line.split()) < 2):
                continue
            results[expt_file].setStats(line, int(size))
        f.close()

Stats.plot_mpki(results, "L1D", block_size)
Stats.plot_hitrate(results, "L1D", block_size)
Stats.plot_utilization(results, "L1D", block_size)
Stats.plot_miss_breakdown(results, "L1D", block_size)
plt.show()
#os.chdir(basePath)

#f = open("results/" + cache + '_' + args.d + "_hits_misses.csv", "w")
#f.write("Benchmark,Accesses,Hits,Misses,Unrealised_Hits_4B_Sector,Unrealised_Hits_8B_Sector,Hit_Rate,Unrealised_Hit_Rate_4B_Sector,Unrealised_Hit_rate_8B_Sector,Compulsory_Misses,Capacity_Misses,Conflict_Misses\n")
#for expt in results.keys():
#    trace = expt[:-24]

#    f.write(f"{trace},{results[expt].cacheStats['way_12'][4].total_accesses},{results[expt].cacheStats['way_12'][4].total_hits},{results[expt].cacheStats['way_12'][4].total_misses},{results[expt].cacheStats['way_12'][4].unrealised_hits},{results[expt].cacheStats['way_12'][8].unrealised_hits},{results[expt].cacheStats['way_12'][4].hit_rate},{results[expt].cacheStats['way_12'][4].unrealised_hit_rate},{results[expt].cacheStats['way_12'][8].unrealised_hit_rate},{results[expt].cacheStats['way_12'][4].compulsory_misses},{results[expt].cacheStats['way_12'][4].capacity_misses},{results[expt].cacheStats['way_12'][4].conflict_misses}")
#    f.write('\n')

#f.close()
            
#f = open("results/" + cache + '_' + args.d + "_util.csv", "w")
#f.write("Benchmark,Way_8_Cache_Line_Unutilized_4B_Sector,Way_8_Cache_Line_Unutilized_8B_Sector,Way_12_Cache_Line_Unutilized_4B_Sector,Way_12_Cache_Line_Unutilized_8B_Sector,Way_16_Cache_Line_Unutilized_4B_Sector,Way_16_Cache_Line_Unutilized_8B_Sector\n")
#for expt in results.keys():
#    trace = expt[:-24]
#    f.write(f"{trace},{results[expt].cacheStats['way_8'][4].cache_line_utilized},{results[expt].cacheStats['way_8'][8].cache_line_utilized},{results[expt].cacheStats['way_12'][4].cache_line_utilized},{results[expt].cacheStats['way_12'][8].cache_line_utilized},{results[expt].cacheStats['way_16'][4].cache_line_utilized},{results[expt].cacheStats['way_16'][8].cache_line_utilized}")
#    f.write('\n')

#f.close()

#f = open("results/" + cache + '_' + args.d + "_max_util.csv", "w")
#f.write("Benchmark,Max_Way_8_Cache_Line_Unutilized_4B_Sector,Max_Way_8_Cache_Line_Unutilized_8B_Sector,Max_Way_12_Cache_Line_Unutilized_4B_Sector,Max_Way_12_Cache_Line_Unutilized_8B_Sector,Max_Way_16_Cache_Line_Unutilized_4B_Sector,Max_Way_16_Cache_Line_Unutilized_8B_Sector\n")
#for expt in results.keys():
#    trace = expt[:-24]
#    f.write(f"{trace},{results[expt].cacheStats['way_8'][4].max_cache_line_utilized},{results[expt].cacheStats['way_8'][8].max_cache_line_utilized},{results[expt].cacheStats['way_12'][4].max_cache_line_utilized},{results[expt].cacheStats['way_12'][8].max_cache_line_utilized},{results[expt].cacheStats['way_16'][4].max_cache_line_utilized},{results[expt].cacheStats['way_16'][8].max_cache_line_utilized}")
#    f.write('\n')

#f.close()
