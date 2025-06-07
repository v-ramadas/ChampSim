#include "cache_stats.h"

cache_stats operator-(cache_stats lhs, cache_stats rhs)
{
  cache_stats result;
  result.pf_requested = lhs.pf_requested - rhs.pf_requested;
  result.pf_issued = lhs.pf_issued - rhs.pf_issued;
  result.pf_useful = lhs.pf_useful - rhs.pf_useful;
  result.pf_useless = lhs.pf_useless - rhs.pf_useless;
  result.pf_fill = lhs.pf_fill - rhs.pf_fill;

  result.hits = lhs.hits - rhs.hits;
  result.misses = lhs.misses - rhs.misses;

  result.evictions = lhs.evictions - rhs.evictions;
  result.compulsory_misses = lhs.compulsory_misses - rhs.compulsory_misses;
  result.capacity_misses = lhs.capacity_misses - rhs.capacity_misses;
  result.unrealised_hits = lhs.unrealised_hits -rhs.unrealised_hits;
  result.no_access_subblocks = lhs.no_access_subblocks - rhs.no_access_subblocks;
  result.total_no_access_subblocks = lhs.total_no_access_subblocks - rhs.total_no_access_subblocks;
  result.total_hits = lhs.total_hits - rhs.total_hits;
  result.total_misses = lhs.total_misses - rhs.total_misses;
  result.total_unrealised_hits = lhs.total_unrealised_hits - rhs.total_unrealised_hits;
  result.total_no_access_subblocks = lhs.total_no_access_subblocks - rhs.total_no_access_subblocks;
  result.total_evictions = lhs.total_evictions - rhs.total_evictions;

  result.total_miss_latency_cycles = lhs.total_miss_latency_cycles - rhs.total_miss_latency_cycles;
  return result;
}
