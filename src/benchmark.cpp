#include "engine.hpp"
#include "scenario_io.hpp"
#include "scenario_path.hpp"

#include <QCoreApplication>
#include <QString>

#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <initializer_list>
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

std::string csv_escape(const std::string& value) {
    std::string escaped = "\"";
    for (char character : value) {
        if (character == '"') {
            escaped += "\"\"";
        } else {
            escaped += character;
        }
    }
    escaped += '"';
    return escaped;
}

void print_help() {
    std::cout
        << "Usage: traffic_benchmark [--scenario FILE] [--steps N] [--repetitions N]\n"
        << "                         [--output FILE.csv]\n"
        << "Runs sequential and OpenMP modes and writes timing/traffic metrics to CSV.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    Q_UNUSED(application);
    try {
        QString scenario_file;
        std::string output_path = "benchmark.csv";
        std::uint64_t steps_override = 0;
        bool has_steps_override = false;
        std::uint32_t repetitions = 3;

        for (int index = 1; index < argc; ++index) {
            const std::string argument(argv[index]);
            if (argument == "--help" || argument == "-h") {
                print_help();
                return 0;
            }
            if (argument != "--scenario" && argument != "--steps"
                && argument != "--repetitions" && argument != "--output") {
                throw std::invalid_argument("Unknown option: " + argument);
            }
            if (index + 1 >= argc) {
                throw std::invalid_argument("Missing value after " + argument);
            }
            const std::string value(argv[++index]);
            if (argument == "--scenario") {
                scenario_file = QString::fromLocal8Bit(value.c_str());
            } else if (argument == "--steps") {
                steps_override = parse_number<std::uint64_t>(value, argument);
                has_steps_override = true;
            } else if (argument == "--repetitions") {
                repetitions = parse_number<std::uint32_t>(value, argument);
            } else {
                output_path = value;
            }
        }
        if (repetitions == 0 || repetitions > 1000) {
            throw std::invalid_argument("repetitions must be between 1 and 1000");
        }

        const auto scenarios = traffic::load_scenarios(
            traffic::resolve_scenario_path(scenario_file));
        traffic::SimulationConfig base_config = scenarios.front().config;
        if (has_steps_override) {
            base_config.steps = steps_override;
        }
        const std::string validation_error = traffic::validate_config(base_config);
        if (!validation_error.empty()) {
            throw std::invalid_argument(validation_error);
        }

        std::ofstream output(output_path, std::ios::out | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Cannot write benchmark output: " + output_path);
        }
        output << "scenario,mode,repeat,steps,elapsed_ms,departed,average_wait_ticks,average_queue,seed\n";
        output << std::fixed << std::setprecision(4);
        for (bool parallel : {false, true}) {
            for (std::uint32_t repetition = 1; repetition <= repetitions; ++repetition) {
                traffic::SimulationConfig config = base_config;
                config.use_openmp = parallel;
                traffic::SimulationEngine engine(config);
                const auto start = std::chrono::steady_clock::now();
                engine.run_steps(config.steps);
                const auto end = std::chrono::steady_clock::now();
                const double elapsed_ms =
                    std::chrono::duration<double, std::milli>(end - start).count();

                output << csv_escape(scenarios.front().name.toStdString()) << ','
                       << (parallel ? "openmp" : "sequential") << ','
                       << repetition << ',' << engine.tick() << ',' << elapsed_ms << ','
                       << engine.metrics().departed() << ','
                       << engine.metrics().average_waiting_time() << ','
                       << engine.metrics().average_queue_length() << ','
                       << config.seed << '\n';
            }
        }
        if (!output) {
            throw std::runtime_error("Failed while writing benchmark output: " + output_path);
        }
        std::cout << "Wrote " << (repetitions * 2) << " benchmark rows to " << output_path << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "traffic_benchmark: " << error.what() << '\n';
        return 1;
    }
}
