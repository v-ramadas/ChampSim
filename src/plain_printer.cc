/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cmath>
#include <numeric>
#include <ratio>
#include <string_view> // for string_view
#include <utility>
#include <vector>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include "stats_printer.h"

namespace
{
template <typename N, typename D>
auto print_ratio(N num, D denom)
{
  if (denom > 0) {
    return fmt::format("{:.4g}", std::ceil(num) / std::ceil(denom));
  }
  return std::string{"-"};
}
} // namespace

std::vector<std::string> champsim::plain_printer::format(O3_CPU::stats_type stats)
{
  constexpr std::array types{branch_type::BRANCH_DIRECT_JUMP, branch_type::BRANCH_INDIRECT,      branch_type::BRANCH_CONDITIONAL,
                             branch_type::BRANCH_DIRECT_CALL, branch_type::BRANCH_INDIRECT_CALL, branch_type::BRANCH_RETURN};
  auto total_branch = std::ceil(
      std::accumulate(std::begin(types), std::end(types), 0LL, [tbt = stats.total_branch_types](auto acc, auto next) { return acc + tbt.value_or(next, 0); }));
  auto total_mispredictions = std::ceil(
      std::accumulate(std::begin(types), std::end(types), 0LL, [btm = stats.branch_type_misses](auto acc, auto next) { return acc + btm.value_or(next, 0); }));

  std::vector<std::string> lines{};
  lines.push_back(fmt::format("{} cumulative IPC: {} instructions: {} cycles: {}", stats.name, ::print_ratio(stats.instrs(), stats.cycles()), stats.instrs(),
                              stats.cycles()));

  lines.push_back(fmt::format("{} Branch Prediction Accuracy: {}% MPKI: {} Average ROB Occupancy at Mispredict: {}", stats.name,
                              ::print_ratio(100 * (total_branch - total_mispredictions), total_branch),
                              ::print_ratio(std::kilo::num * total_mispredictions, stats.instrs()),
                              ::print_ratio(stats.total_rob_occupancy_at_branch_mispredict, total_mispredictions)));

  lines.emplace_back("Branch type MPKI");
  for (auto idx : types) {
    lines.push_back(fmt::format("{}: {}", branch_type_names.at(champsim::to_underlying(idx)),
                                ::print_ratio(std::kilo::num * stats.branch_type_misses.value_or(idx, 0), stats.instrs())));
  }

  return lines;
}

std::vector<std::string> champsim::plain_printer::format(CACHE::stats_type stats)
{
  using hits_value_type = typename decltype(stats.hits)::value_type;
  using misses_value_type = typename decltype(stats.misses)::value_type;
  using mshr_merge_value_type = typename decltype(stats.mshr_merge)::value_type;
  using mshr_return_value_type = typename decltype(stats.mshr_return)::value_type;
  using evictions_value_type = typename decltype(stats.evictions)::value_type;
  using compulsory_misses_value_type = typename decltype(stats.compulsory_misses)::value_type;
  using capacity_misses_value_type = typename decltype(stats.capacity_misses)::value_type;
  using conflict_misses_value_type = typename decltype(stats.capacity_misses)::value_type;
  using unrealised_hits_value_type = typename decltype(stats.unrealised_hits)::value_type;
  using total_unrealised_hits_value_type = typename decltype(stats.total_unrealised_hits)::value_type;
  using no_access_subblocks_value_type = typename decltype(stats.no_access_subblocks)::value_type;
  using total_no_access_subblocks_value_type = typename decltype(stats.total_no_access_subblocks)::value_type;
  using total_hits_value_type = typename decltype(stats.total_hits)::value_type;
  using total_misses_value_type = typename decltype(stats.total_misses)::value_type;
  using total_evictions_value_type = typename decltype(stats.total_evictions)::value_type;
  using evictions_breakdown_value_type = typename decltype(stats.total_evictions)::value_type;

  std::vector<std::size_t> cpus;
  uint64_t num_blocks = stats.evictions_breakdown.size();

  // build a vector of all existing cpus
  auto stat_keys = {stats.hits.get_keys(), stats.misses.get_keys(), stats.mshr_merge.get_keys(), stats.mshr_return.get_keys()};
  for (auto keys : stat_keys) {
    std::transform(std::begin(keys), std::end(keys), std::back_inserter(cpus), [](auto val) { return val.second; });
  }
  std::sort(std::begin(cpus), std::end(cpus));
  auto uniq_end = std::unique(std::begin(cpus), std::end(cpus));
  cpus.erase(uniq_end, std::end(cpus));

  for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION}) {
    for (auto cpu : cpus) {
      stats.hits.allocate(std::pair{type, cpu});
      stats.misses.allocate(std::pair{type, cpu});
      stats.mshr_merge.allocate(std::pair{type, cpu});
      stats.mshr_return.allocate(std::pair{type, cpu});
      stats.no_access_subblocks.allocate(std::pair{type, cpu});
      stats.evictions.allocate(std::pair{type, cpu});
      stats.compulsory_misses.allocate(std::pair{type, cpu});
      stats.unrealised_hits.allocate(std::pair{type, cpu});
      stats.total_hits.allocate(std::pair{type, cpu});
      stats.total_misses.allocate(std::pair{type, cpu});
      stats.total_unrealised_hits.allocate(std::pair{type, cpu});
      stats.total_no_access_subblocks.allocate(std::pair{type, cpu});
      stats.total_evictions.allocate(std::pair{type, cpu});
      for (auto it = stats.evictions_breakdown.begin(); it != stats.evictions_breakdown.end(); it++) {
          it->allocate(std::pair{type, cpu});
      }
    }
  }

  std::vector<std::string> lines{};
  for (auto cpu : cpus) {
    hits_value_type total_hits = 0;
    misses_value_type total_misses = 0;
    mshr_merge_value_type total_mshr_merge = 0;
    mshr_return_value_type total_mshr_return = 0;
    evictions_value_type total_evictions = 0;
    compulsory_misses_value_type compulsory_misses = 0;
    capacity_misses_value_type capacity_misses = 0;
    conflict_misses_value_type conflict_misses = 0;
    unrealised_hits_value_type unrealised_hits = 0;
    no_access_subblocks_value_type no_access_subblocks = 0;
    total_hits_value_type full_sim_hits = 0;
    total_misses_value_type full_sim_misses = 0;
    total_unrealised_hits_value_type full_sim_unrealised_hits = 0;
    total_no_access_subblocks_value_type full_sim_no_access_subblocks = 0;
    total_evictions_value_type full_sim_evictions = 0;
    std::vector<evictions_breakdown_value_type> evictions_breakdown(num_blocks, 0);

    for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE}) {
      total_hits += stats.hits.value_or(std::pair{type, cpu}, hits_value_type{});
      total_misses += stats.misses.value_or(std::pair{type, cpu}, misses_value_type{});
      total_mshr_merge += stats.mshr_merge.value_or(std::pair{type, cpu}, mshr_merge_value_type{});
      total_mshr_return += stats.mshr_return.value_or(std::pair{type, cpu}, mshr_merge_value_type{});
      total_evictions += stats.evictions.value_or(std::pair{type, cpu}, evictions_value_type{});
      compulsory_misses += stats.compulsory_misses.value_or(std::pair{type, cpu}, compulsory_misses_value_type{});
      capacity_misses += stats.capacity_misses.value_or(std::pair{type, cpu}, capacity_misses_value_type{});
      unrealised_hits += stats.unrealised_hits.value_or(std::pair{type, cpu}, unrealised_hits_value_type{});
      no_access_subblocks += stats.no_access_subblocks.value_or(std::pair{type, cpu}, no_access_subblocks_value_type{});
      full_sim_hits += stats.total_hits.value_or(std::pair{type, cpu}, total_hits_value_type{});
      full_sim_misses += stats.total_misses.value_or(std::pair{type, cpu}, total_misses_value_type{});
      full_sim_unrealised_hits += stats.total_unrealised_hits.value_or(std::pair{type, cpu}, total_unrealised_hits_value_type{});
      full_sim_no_access_subblocks += stats.total_no_access_subblocks.value_or(std::pair{type, cpu}, total_no_access_subblocks_value_type{});
      full_sim_evictions += stats.total_evictions.value_or(std::pair{type, cpu}, total_evictions_value_type{});
      for (uint64_t i = 0; i < num_blocks; i++) {
          evictions_breakdown[i] += stats.evictions_breakdown[i].value_or(std::pair{type, cpu}, evictions_breakdown_value_type{});
      }
    }

    if (ENABLE_MISS_BREAKDOWN) {
      capacity_misses -= compulsory_misses;
      conflict_misses = (full_sim_misses-total_mshr_merge) - compulsory_misses - capacity_misses;
    }

    fmt::format_string<std::string_view, std::string_view, int, int, int> hitmiss_fmtstr{
        "cpu{}->{} {:<12s} ACCESS: {:10d} HIT: {:10d} MISS: {:10d} COMPULSORY_MISS: {:10d} CAPACITY_MISS: {:10d} CONFLICT_MISS: {:10d} MSHR_MERGE: {:10d}"};
    fmt::format_string<std::string_view, std::string_view, int, int, int> ghost_cache_fmtstr{
        "cpu{}->{} {:<12s} TOTAL_ACCESS: {:10d} TOTAL_HIT: {:10d} TOTAL_MISS: {:10d} UNREALISED_HIT: {:10d} TOTAL_UNREALISED_HIT: {:10d} UNREALISED_HIT_RATE: {:10f} TOTAL_UNREALISED_HIT_RATE: {:10f}"};
    fmt::format_string<std::string_view, std::string_view, int, int, int> subblock_access_fmtstr{
        "cpu{}->{} {:<12s} EVICTIONS: {:10d} TOTAL_EVICTIONS: {:10d} SUB_BLOCKS_UNACCESSED: {:10d} TOTAL_SUB_BLOCKS_UNACCESSED: {:10d}"};

    lines.push_back(fmt::format(hitmiss_fmtstr, cpu, stats.name, "TOTAL", total_hits + total_misses, total_hits, total_misses, compulsory_misses, capacity_misses, conflict_misses, total_mshr_merge));
    lines.push_back(fmt::format(subblock_access_fmtstr, cpu, stats.name, "TOTAL", total_evictions, full_sim_evictions, no_access_subblocks, full_sim_no_access_subblocks));
    lines.push_back(fmt::format(ghost_cache_fmtstr, cpu, stats.name, "TOTAL", full_sim_hits + full_sim_misses, full_sim_hits, full_sim_misses, unrealised_hits, full_sim_unrealised_hits, float(total_hits + unrealised_hits)/float(total_hits+total_misses), float(full_sim_hits + full_sim_unrealised_hits)/float(full_sim_hits+full_sim_misses)));
    switch(num_blocks) {
        case 1:
            {
                fmt::format_string<std::string_view, std::string_view, int, int, int, int> evictions_breakdown_fmtstr{
                    "cpu{}->{} {:<12s} EVICTIONS_BREAKDOWN BLOCKS_USED 1: {:10d}"};
                lines.push_back(fmt::format(evictions_breakdown_fmtstr, cpu, stats.name, "TOTAL",
                    evictions_breakdown[0]));
                    break;
            }
        case 2:
            {
                fmt::format_string<std::string_view, std::string_view, int, int, int, int> evictions_breakdown_fmtstr{
                    "cpu{}->{} {:<12s} EVICTIONS_BREAKDOWN BLOCKS_USED 1: {:10d} 2: {:10d}"};
                lines.push_back(fmt::format(evictions_breakdown_fmtstr, cpu, stats.name, "TOTAL",
                    evictions_breakdown[0], evictions_breakdown[1]));
                break;
            }
        case 4:
            {
                fmt::format_string<std::string_view, std::string_view, int, int, int, int> evictions_breakdown_fmtstr{
                    "cpu{}->{} {:<12s} EVICTIONS_BREAKDOWN BLOCKS_USED 1: {:10d} 2: {:10d} 3: {:10d} 4: {:10d}"};
                lines.push_back(fmt::format(evictions_breakdown_fmtstr, cpu, stats.name, "TOTAL",
                    evictions_breakdown[0], evictions_breakdown[1], evictions_breakdown[2], evictions_breakdown[3]));
                break;
            }
        case 8:
            {
                fmt::format_string<std::string_view, std::string_view, int, int, int, int, int, int, int, int> evictions_breakdown_fmtstr{
                    "cpu{}->{} {:<12s} EVICTIONS_BREAKDOWN BLOCKS_USED 1: {:10d} 2: {:10d} 3: {:10d} 4: {:10d} 5: {:10d} 6: {:10d} 7: {:10d} 8: {:10d}"};
                lines.push_back(fmt::format(evictions_breakdown_fmtstr, cpu, stats.name, "TOTAL",
                    evictions_breakdown[0], evictions_breakdown[1], evictions_breakdown[2], evictions_breakdown[3],
                    evictions_breakdown[4], evictions_breakdown[5], evictions_breakdown[6], evictions_breakdown[7]));
                break;
            }
        case 16:
            {
                fmt::format_string<std::string_view, std::string_view, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int> evictions_breakdown_fmtstr{
                    "cpu{}->{} {:<12s} EVICTIONS_BREAKDOWN BLOCKS_USED 1: {:10d} 2: {:10d} 3: {:10d} 4: {:10d} 5: {:10d} 6: {:10d} 7: {:10d} 8: {:10d} 9: {:10d} 10: {:10d} 11: {:10d} 12: {:10d} 13: {:10d} 14: {:10d} 15: {:10d} 16: {:10d}"};
                lines.push_back(fmt::format(evictions_breakdown_fmtstr, cpu, stats.name, "TOTAL",
                    evictions_breakdown[0], evictions_breakdown[1], evictions_breakdown[2], evictions_breakdown[3],
                    evictions_breakdown[4], evictions_breakdown[5], evictions_breakdown[6], evictions_breakdown[7],
                    evictions_breakdown[8], evictions_breakdown[9], evictions_breakdown[10], evictions_breakdown[11],
                    evictions_breakdown[12], evictions_breakdown[13], evictions_breakdown[14], evictions_breakdown[15]));
                break;
            }
        default:
            assert(false);
            break;
    }



    for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION}) {
      hits_value_type per_type_total_hits = stats.hits.value_or(std::pair{type, cpu}, hits_value_type{});
      misses_value_type per_type_total_misses = stats.misses.value_or(std::pair{type, cpu}, misses_value_type{});
      mshr_merge_value_type per_type_total_mshr_merge = stats.mshr_merge.value_or(std::pair{type, cpu}, mshr_merge_value_type{});
      evictions_value_type per_type_total_evictions = stats.evictions.value_or(std::pair{type, cpu}, evictions_value_type{});
      compulsory_misses_value_type per_type_compulsory_misses = stats.compulsory_misses.value_or(std::pair{type, cpu}, compulsory_misses_value_type{});
      capacity_misses_value_type per_type_capacity_misses = stats.capacity_misses.value_or(std::pair{type, cpu}, capacity_misses_value_type{}) - compulsory_misses;
      unrealised_hits_value_type per_type_unrealised_hits = stats.unrealised_hits.value_or(std::pair{type, cpu}, unrealised_hits_value_type{});
      no_access_subblocks_value_type per_type_no_access_subblocks = stats.no_access_subblocks.value_or(std::pair{type, cpu}, no_access_subblocks_value_type{});
      total_hits_value_type per_type_full_sim_hits = stats.total_hits.value_or(std::pair{type, cpu}, total_hits_value_type{});
      total_misses_value_type per_type_full_sim_misses = stats.total_misses.value_or(std::pair{type, cpu}, total_misses_value_type{});
      total_unrealised_hits_value_type per_type_full_sim_unrealised_hits = stats.total_unrealised_hits.value_or(std::pair{type, cpu}, total_unrealised_hits_value_type{});
      total_evictions_value_type per_type_full_sim_evictions = stats.total_evictions.value_or(std::pair{type, cpu}, total_evictions_value_type{});
      total_no_access_subblocks_value_type per_type_full_sim_no_access_subblocks = stats.total_no_access_subblocks.value_or(std::pair{type, cpu}, total_no_access_subblocks_value_type{});
      conflict_misses_value_type per_type_conflict_misses = 0;
      if (ENABLE_MISS_BREAKDOWN) {
        per_type_capacity_misses -= per_type_compulsory_misses;
        per_type_conflict_misses = (per_type_full_sim_misses - per_type_total_mshr_merge) - per_type_compulsory_misses - per_type_capacity_misses;
      }  

      lines.push_back(
          fmt::format(hitmiss_fmtstr, cpu, stats.name, access_type_names.at(champsim::to_underlying(type)),
                      stats.hits.value_or(std::pair{type, cpu}, hits_value_type{}) + stats.misses.value_or(std::pair{type, cpu}, misses_value_type{}),
                      stats.hits.value_or(std::pair{type, cpu}, hits_value_type{}), stats.misses.value_or(std::pair{type, cpu}, misses_value_type{}),
                      per_type_compulsory_misses, per_type_capacity_misses, per_type_conflict_misses,
                      stats.mshr_merge.value_or(std::pair{type, cpu}, mshr_merge_value_type{})));
      lines.push_back(fmt::format(subblock_access_fmtstr, cpu, stats.name, access_type_names.at(champsim::to_underlying(type)),
                  per_type_total_evictions, per_type_full_sim_evictions, per_type_no_access_subblocks, per_type_full_sim_no_access_subblocks));
      lines.push_back(fmt::format(ghost_cache_fmtstr, cpu, stats.name, access_type_names.at(champsim::to_underlying(type)),
                  per_type_full_sim_hits + per_type_full_sim_misses, per_type_full_sim_hits, per_type_full_sim_misses,
                  per_type_unrealised_hits, per_type_full_sim_unrealised_hits,
                  float(per_type_total_hits + per_type_unrealised_hits)/float(per_type_total_hits+per_type_total_misses),
                  float(per_type_full_sim_hits + per_type_full_sim_unrealised_hits)/float(per_type_full_sim_hits+per_type_full_sim_misses)));

    lines.push_back(fmt::format("cpu{}->{} PREFETCH REQUESTED: {:10} ISSUED: {:10} USEFUL: {:10} USELESS: {:10}", cpu, stats.name, stats.pf_requested,
                                stats.pf_issued, stats.pf_useful, stats.pf_useless));

    uint64_t total_downstream_demands = total_mshr_return - stats.mshr_return.value_or(std::pair{access_type::PREFETCH, cpu}, mshr_return_value_type{});
    lines.push_back(
        fmt::format("cpu{}->{} AVERAGE MISS LATENCY: {} cycles", cpu, stats.name, ::print_ratio(stats.total_miss_latency_cycles, total_downstream_demands)));
  }
  }

  return lines;
}

std::vector<std::string> champsim::plain_printer::format(DRAM_CHANNEL::stats_type stats)
{
  std::vector<std::string> lines{};
  lines.push_back(fmt::format("{} RQ ROW_BUFFER_HIT: {:10}", stats.name, stats.RQ_ROW_BUFFER_HIT));
  lines.push_back(fmt::format("  ROW_BUFFER_MISS: {:10}", stats.RQ_ROW_BUFFER_MISS));
  lines.push_back(fmt::format("  AVG DBUS CONGESTED CYCLE: {}", ::print_ratio(stats.dbus_cycle_congested, stats.dbus_count_congested)));
  lines.push_back(fmt::format("{} WQ ROW_BUFFER_HIT: {:10}", stats.name, stats.WQ_ROW_BUFFER_HIT));
  lines.push_back(fmt::format("  ROW_BUFFER_MISS: {:10}", stats.WQ_ROW_BUFFER_MISS));
  lines.push_back(fmt::format("  FULL: {:10}", stats.WQ_FULL));

  if (stats.refresh_cycles > 0)
    lines.push_back(fmt::format("{} REFRESHES ISSUED: {:10}", stats.name, stats.refresh_cycles));
  else
    lines.push_back(fmt::format("{} REFRESHES ISSUED: -", stats.name));

  return lines;
}

void champsim::plain_printer::print(champsim::phase_stats& stats)
{
  auto lines = format(stats);
  std::copy(std::begin(lines), std::end(lines), std::ostream_iterator<std::string>(stream, "\n"));
}

std::vector<std::string> champsim::plain_printer::format(champsim::phase_stats& stats)
{
  std::vector<std::string> lines{};
  lines.push_back(fmt::format("=== {} ===", stats.name));

  int i = 0;
  for (auto tn : stats.trace_names) {
    lines.push_back(fmt::format("CPU {} runs {}", i++, tn));
  }

  if (NUM_CPUS > 1) {
    lines.emplace_back("");
    lines.emplace_back("Total Simulation Statistics (not including warmup)");

    for (const auto& stat : stats.sim_cpu_stats) {
      auto sublines = format(stat);
      lines.emplace_back("");
      std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
      lines.emplace_back("");
    }

    for (const auto& stat : stats.sim_cache_stats) {
      auto sublines = format(stat);
      std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
    }
  }

  lines.emplace_back("");
  lines.emplace_back("Region of Interest Statistics");

  for (const auto& stat : stats.roi_cpu_stats) {
    auto sublines = format(stat);
    lines.emplace_back("");
    std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
    lines.emplace_back("");
  }

  for (const auto& stat : stats.roi_cache_stats) {
    auto sublines = format(stat);
    std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
  }

  lines.emplace_back("");
  lines.emplace_back("DRAM Statistics");
  for (const auto& stat : stats.roi_dram_stats) {
    auto sublines = format(stat);
    lines.emplace_back("");
    std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
  }

  return lines;
}

void champsim::plain_printer::print(std::vector<phase_stats>& stats)
{
  for (auto p : stats) {
    print(p);
  }
}
