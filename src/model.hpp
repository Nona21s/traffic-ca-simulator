#pragma once

#include "config.hpp"
#include "types.hpp"

namespace traffic {

inline SignalPhase phase_at(std::uint64_t tick, const SimulationConfig& config) noexcept {
    const std::uint64_t ns = config.north_south_green_ticks;
    const std::uint64_t ew = config.east_west_green_ticks;
    return tick % (ns + ew) < ns ? SignalPhase::NorthSouth : SignalPhase::EastWest;
}

inline bool is_green(Approach approach, SignalPhase phase) noexcept {
    const bool is_north_south = approach == Approach::North || approach == Approach::South;
    return is_north_south == (phase == SignalPhase::NorthSouth);
}

} // namespace traffic
