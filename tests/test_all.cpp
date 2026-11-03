#include "config.hpp"
#include "engine.hpp"
#include "model.hpp"

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_configuration_validation() {
    traffic::SimulationConfig config;
    require(traffic::validate_config(config).empty(), "default configuration should be valid");

    config.arrival_rate = 1.1;
    require(!traffic::validate_config(config).empty(), "arrival rate above 1 should be rejected");

    config.arrival_rate = 0.2;
    config.approach_length = 1;
    require(!traffic::validate_config(config).empty(), "approach shorter than 2 cells should be rejected");
}

void test_fixed_signal_cycle() {
    traffic::SimulationConfig config;
    config.north_south_green_ticks = 2;
    config.east_west_green_ticks = 3;
    require(traffic::phase_at(0, config) == traffic::SignalPhase::NorthSouth, "cycle should start N/S green");
    require(traffic::phase_at(1, config) == traffic::SignalPhase::NorthSouth, "N/S phase duration should be honored");
    require(traffic::phase_at(2, config) == traffic::SignalPhase::EastWest, "E/W phase should follow N/S");
    require(traffic::phase_at(4, config) == traffic::SignalPhase::EastWest, "E/W phase duration should be honored");
    require(traffic::phase_at(5, config) == traffic::SignalPhase::NorthSouth, "signal cycle should repeat");
}

void test_red_signal_holds_vehicles() {
    traffic::SimulationConfig config;
    config.approach_length = 2;
    config.steps = 1;
    config.arrival_rate = 0.0;
    config.initial_density = 1.0;
    traffic::SimulationEngine engine(config);

    engine.step();

    const auto& east_lane = engine.grid().lane(traffic::Approach::East);
    require(east_lane[0].has_value(), "front vehicle should remain at the red signal");
    require(east_lane[0]->waiting_ticks == 1, "front vehicle should accrue one waiting tick");
    require(engine.metrics().departed() == 2, "only the two green N/S approaches should depart");
}

void test_green_signal_releases_front_vehicle() {
    traffic::SimulationConfig config;
    config.approach_length = 2;
    config.steps = 1;
    config.arrival_rate = 0.0;
    config.initial_density = 1.0;
    traffic::SimulationEngine engine(config);

    engine.step();

    require(engine.metrics().departed() == 2, "one vehicle from each green approach should depart");
    require(engine.grid().lane(traffic::Approach::North)[0].has_value(),
            "following vehicle should advance into the vacated stop cell");
    require(engine.grid().vehicle_count() == 6, "two vehicles should have left the intersection");
}

void test_seed_reproducibility() {
    traffic::SimulationConfig config;
    config.steps = 160;
    config.seed = 987654;

    traffic::SimulationEngine first(config);
    traffic::SimulationEngine second(config);
    first.run_steps(config.steps);
    second.run_steps(config.steps);

    require(first.state_hash() == second.state_hash(), "same seed should reproduce the same state");
    require(first.metrics().departed() == second.metrics().departed(),
            "same seed should reproduce throughput");
    require(first.metrics().average_waiting_time() == second.metrics().average_waiting_time(),
            "same seed should reproduce average wait");
}

void test_openmp_matches_sequential() {
    traffic::SimulationConfig sequential_config;
    sequential_config.steps = 250;
    sequential_config.seed = 13579;
    traffic::SimulationConfig parallel_config = sequential_config;
    parallel_config.use_openmp = true;

    traffic::SimulationEngine sequential(sequential_config);
    traffic::SimulationEngine parallel(parallel_config);
    sequential.run_steps(sequential_config.steps);
    parallel.run_steps(parallel_config.steps);

    require(sequential.grid().same_state(parallel.grid()),
            "OpenMP and sequential modes should have identical cell state");
    require(sequential.metrics().departed() == parallel.metrics().departed(),
            "OpenMP and sequential modes should have identical throughput");
    require(sequential.metrics().average_waiting_time() == parallel.metrics().average_waiting_time(),
            "OpenMP and sequential modes should have identical average wait");
}

void test_empty_scenario_stays_empty() {
    traffic::SimulationConfig config;
    config.steps = 40;
    config.arrival_rate = 0.0;
    config.initial_density = 0.0;
    traffic::SimulationEngine engine(config);
    engine.run_steps(config.steps);
    require(engine.grid().vehicle_count() == 0, "zero density and zero arrivals should remain empty");
    require(engine.metrics().departed() == 0, "empty scenario should have no throughput");
    require(engine.metrics().average_queue_length() == 0.0, "empty scenario should have zero average queue");
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"configuration validation", test_configuration_validation},
        {"fixed signal cycle", test_fixed_signal_cycle},
        {"red signal", test_red_signal_holds_vehicles},
        {"green signal", test_green_signal_releases_front_vehicle},
        {"seed reproducibility", test_seed_reproducibility},
        {"OpenMP parity", test_openmp_matches_sequential},
        {"empty scenario", test_empty_scenario_stays_empty}
    };

    std::size_t failed = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        } catch (const std::exception& error) {
            ++failed;
            std::cerr << "[FAIL] " << test.first << ": " << error.what() << '\n';
        }
    }
    std::cout << tests.size() - failed << '/' << tests.size() << " tests passed\n";
    return failed == 0 ? 0 : 1;
}
