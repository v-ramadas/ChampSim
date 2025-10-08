#include "nlru_1.h"

#include <algorithm>
#include <cassert>
#include <climits>

nlru_1::nlru_1(CACHE* cache) : nlru_1(cache, cache->NUM_SET, cache->NUM_WAY) {}

nlru_1::nlru_1(CACHE* cache, long sets, long ways) : replacement(cache), NUM_WAY(ways), last_used_cycles(static_cast<std::size_t>(sets * ways), 0) {}

long nlru_1::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                      champsim::address full_addr, access_type type)
{
  auto begin = std::next(std::begin(last_used_cycles), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  // Find the way whose last use cycle is most distant
  auto victim = std::min_element(begin, end);
  assert(begin <= victim);
  assert(victim < end);
  long victim_idx = std::distance(begin, victim);
  *victim = ULLONG_MAX;
  return victim_idx;
}

void nlru_1::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                 access_type type)
{
  // Mark the way as being used on the current cycle
  if (victim_addr == champsim::address{0}) {
      // Components of fill not requested
      if (lru_counter >= 1000)
          lru_counter = 0;

      last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = lru_counter++;
  } else {
      last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = mru_counter++;
  }
}

void nlru_1::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type, uint8_t hit)
{
  // Mark the way as being used on the current cycle
  if (hit && access_type{type} != access_type::WRITE) // Skip this for writeback hits
    last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = mru_counter++;
}
