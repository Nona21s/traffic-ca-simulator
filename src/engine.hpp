#pragma once

#include "config.hpp"
#include "grid.hpp"
#include "metrics.hpp"
#include "model.hpp"

#include <cstdint>

namespace traffic {

class SimulationEngine {
public:
    explicit SimulationEngine(SimulationConfig config);

    void step();
    void run_steps(std::uint64_t count);

    const SimulationConfig& config() const noexcept;
    const Grid& grid() const noexcept;
    const TrafficMetrics& metrics() const noexcept;
    std::uint64_t tick() const noexcept;
    SignalPhase current_phase() const noexcept;
    std::uint64_t state_hash() const noexcept;

private:
    SimulationConfig config_;
    Grid current_;
    Grid next_;
    TrafficMetrics metrics_;
    std::uint64_t tick_ = 0;
};

} // namespace traffic
