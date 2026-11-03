#include "engine.hpp"
#include "scenario_io.hpp"
#include "scenario_path.hpp"

#include <QCoreApplication>
#include <QString>

#include <exception>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace {

template <typename Number>
Number parse_number(const std::string& value, const std::string& option) {
    if constexpr (!std::is_floating_point<Number>::value) {
        if (!value.empty() && value.front() == '-') {
            throw std::invalid_argument("Invalid value for " + option + ": " + value);
        }
    }
    std::size_t parsed = 0;
    Number result{};
    try {
        if constexpr (std::is_floating_point<Number>::value) {
            result = static_cast<Number>(std::stod(value, &parsed));
        } else {
            const unsigned long long parsed_number = std::stoull(value, &parsed);
            if (parsed_number > static_cast<unsigned long long>(std::numeric_limits<Number>::max())) {
                throw std::out_of_range("numeric value is too large");
            }
            result = static_cast<Number>(parsed_number);
        }
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid value for " + option + ": " + value);
    }
    if (parsed != value.size()) {
        throw std::invalid_argument("Invalid value for " + option + ": " + value);
    }
    return result;
}

void print_help() {
    std::cout
        << "Traffic cellular automaton simulator\n"
        << "Usage: traffic_ca [--scenario FILE] [--steps N] [--seed N]\n"
        << "                  [--arrival-rate P] [--parallel]\n"
        << "Options:\n"
        << "  --scenario FILE   Load the first scenario from a JSON file\n"
        << "  --steps N         Override the number of simulation steps\n"
        << "  --seed N          Override the random seed\n"
        << "  --arrival-rate P  Override vehicle arrival probability (0..1)\n"
        << "  --parallel        Run lane updates with OpenMP\n"
        << "  --help            Show this help\n";
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    Q_UNUSED(application);
    try {
        QString scenario_file;
        traffic::SimulationConfig overrides;
        bool has_steps = false;
        bool has_seed = false;
        bool has_arrival_rate = false;
        bool use_parallel = false;

        for (int index = 1; index < argc; ++index) {
            const std::string argument(argv[index]);
            if (argument == "--help" || argument == "-h") {
                print_help();
                return 0;
            }
            if (argument == "--parallel") {
                use_parallel = true;
                continue;
            }
            if (argument == "--scenario" || argument == "--steps" || argument == "--seed"
                || argument == "--arrival-rate") {
                if (index + 1 >= argc) {
                    throw std::invalid_argument("Missing value after " + argument);
                }
                const std::string value(argv[++index]);
                if (argument == "--scenario") {
                    scenario_file = QString::fromLocal8Bit(value.c_str());
                } else if (argument == "--steps") {
                    overrides.steps = parse_number<std::uint64_t>(value, argument);
                    has_steps = true;
                } else if (argument == "--seed") {
                    overrides.seed = parse_number<std::uint64_t>(value, argument);
                    has_seed = true;
                } else {
                    overrides.arrival_rate = parse_number<double>(value, argument);
                    has_arrival_rate = true;
                }
                continue;
            }
            throw std::invalid_argument("Unknown option: " + argument);
        }

        const auto scenarios = traffic::load_scenarios(
            traffic::resolve_scenario_path(scenario_file));
        traffic::SimulationConfig config = scenarios.front().config;
        if (has_steps) {
            config.steps = overrides.steps;
        }
        if (has_seed) {
            config.seed = overrides.seed;
        }
        if (has_arrival_rate) {
            config.arrival_rate = overrides.arrival_rate;
        }
        config.use_openmp = use_parallel;

        const std::string validation_error = traffic::validate_config(config);
        if (!validation_error.empty()) {
            throw std::invalid_argument(validation_error);
        }

        traffic::SimulationEngine engine(config);
        engine.run_steps(config.steps);
        std::cout << "Scenario: " << scenarios.front().name.toStdString() << '\n'
                  << "Mode: " << (config.use_openmp ? "OpenMP" : "sequential") << '\n'
                  << "Steps: " << engine.tick() << '\n'
                  << "Departed vehicles: " << engine.metrics().departed() << '\n'
                  << "Average wait of departed vehicles (ticks): "
                  << engine.metrics().average_waiting_time() << '\n'
                  << "Average queue length (vehicles): "
                  << engine.metrics().average_queue_length() << '\n'
                  << "Vehicles remaining: " << engine.grid().vehicle_count() << '\n'
                  << "State hash: " << engine.state_hash() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "traffic_ca: " << error.what() << '\n';
        return 1;
    }
}
