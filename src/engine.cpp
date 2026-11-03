#include "engine.hpp"

#include "model.hpp"
#include "rng.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace traffic {
namespace {

SimulationConfig checked_config(SimulationConfig config) {
    const std::string error = validate_config(config);
    if (!error.empty()) {
        throw std::invalid_argument(error);
    }
#ifndef _OPENMP
    if (config.use_openmp) {
        throw std::runtime_error("OpenMP was requested, but this build does not include OpenMP support");
    }
#endif
    return config;
}

} // namespace

SimulationEngine::SimulationEngine(SimulationConfig config)
    : config_(checked_config(config)),
      current_(config_.approach_length, config_.initial_density, config_.seed),
      next_(config_.approach_length, 0.0, config_.seed) {}

void SimulationEngine::step() {
    next_.clear();
    std::array<std::uint64_t, 4> departures{};
    std::array<std::uint64_t, 4> departure_waits{};
    const SignalPhase phase = phase_at(tick_, config_);

#ifdef _OPENMP
#pragma omp parallel for schedule(static) if(config_.use_openmp)
#endif
    for (int direction_index = 0; direction_index < 4; ++direction_index) {
        const auto direction = static_cast<Approach>(direction_index);
        const Lane& source = current_.lane(direction);
        Lane& destination = next_.lane(direction);

        for (std::size_t cell_index = 0; cell_index < source.size(); ++cell_index) {
            if (!source[cell_index].has_value()) {
                continue;
            }

            Vehicle vehicle = *source[cell_index];
            if (cell_index == 0) {
                if (is_green(direction, phase)) {
                    ++departures[static_cast<std::size_t>(direction_index)];
                    departure_waits[static_cast<std::size_t>(direction_index)] += vehicle.waiting_ticks;
                    continue;
                }
                ++vehicle.waiting_ticks;
                destination[cell_index] = vehicle;
                continue;
            }

            const std::size_t next_cell = cell_index - 1;
            if (!destination[next_cell].has_value()) {
                destination[next_cell] = vehicle;
            } else {
                ++vehicle.waiting_ticks;
                destination[cell_index] = vehicle;
            }
        }

        const std::uint64_t arrival_key = 0x200000000ULL
            + tick_ * 4ULL + static_cast<std::uint64_t>(direction_index);
        if (!destination.back().has_value()
            && rng::event(config_.seed, arrival_key, config_.arrival_rate)) {
            destination.back() = Vehicle{};
        }
    }

    for (std::size_t direction = 0; direction < departures.size(); ++direction) {
        metrics_.record_departures(departures[direction], departure_waits[direction]);
    }
    current_.swap(next_);
    metrics_.record_queue(static_cast<std::uint64_t>(current_.vehicle_count()));
    ++tick_;
}

void SimulationEngine::run_steps(std::uint64_t count) {
    for (std::uint64_t step_index = 0; step_index < count; ++step_index) {
        step();
    }
}

const SimulationConfig& SimulationEngine::config() const noexcept {
    return config_;
}

const Grid& SimulationEngine::grid() const noexcept {
    return current_;
}

const TrafficMetrics& SimulationEngine::metrics() const noexcept {
    return metrics_;
}

std::uint64_t SimulationEngine::tick() const noexcept {
    return tick_;
}

SignalPhase SimulationEngine::current_phase() const noexcept {
    return phase_at(tick_, config_);
}

std::uint64_t SimulationEngine::state_hash() const noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    const auto mix_byte = [&hash](std::uint8_t byte) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    };

    for (unsigned int shift = 0; shift < 64; shift += 8) {
        mix_byte(static_cast<std::uint8_t>((tick_ >> shift) & 0xffU));
    }
    for (Approach direction : all_approaches) {
        for (const auto& cell : current_.lane(direction)) {
            mix_byte(cell.has_value() ? 1U : 0U);
            if (cell.has_value()) {
                for (unsigned int shift = 0; shift < 64; shift += 8) {
                    mix_byte(static_cast<std::uint8_t>((cell->waiting_ticks >> shift) & 0xffU));
                }
            }
        }
    }
    return hash;
}

} // namespace traffic
