#pragma once

#include <cstdint>
#include <string>

namespace traffic {

struct SimulationConfig {
    std::uint32_t approach_length = 24;
    std::uint64_t steps = 1000;
    double arrival_rate = 0.22;
    double initial_density = 0.08;
    std::uint64_t seed = 42;
    std::uint32_t north_south_green_ticks = 30;
    std::uint32_t east_west_green_ticks = 30;
    bool use_openmp = false;
};

inline std::string validate_config(const SimulationConfig& config) {
    if (config.approach_length < 2 || config.approach_length > 500) {
        return "approach_length must be between 2 and 500 cells";
    }
    if (config.steps == 0 || config.steps > 10000000) {
        return "steps must be between 1 and 10000000";
    }
    if (!(config.arrival_rate >= 0.0 && config.arrival_rate <= 1.0)) {
        return "arrival_rate must be between 0 and 1";
    }
    if (!(config.initial_density >= 0.0 && config.initial_density <= 1.0)) {
        return "initial_density must be between 0 and 1";
    }
    if (config.north_south_green_ticks == 0 || config.north_south_green_ticks > 1000000
        || config.east_west_green_ticks == 0 || config.east_west_green_ticks > 1000000) {
        return "both green phase durations must be between 1 and 1000000 ticks";
    }
    return {};
}

} // namespace traffic
