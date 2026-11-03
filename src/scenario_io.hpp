#pragma once

#include "config.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace traffic {

struct NamedScenario {
    QString name;
    SimulationConfig config;
};

inline std::vector<NamedScenario> load_scenarios(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(
            "Cannot open scenario file: " + path.toStdString() + " (" + file.errorString().toStdString() + ")");
    }

    QJsonParseError parse_error{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        throw std::runtime_error(
            "Invalid scenario JSON: " + parse_error.errorString().toStdString());
    }

    const QJsonArray scenarios = document.object().value("scenarios").toArray();
    if (scenarios.isEmpty()) {
        throw std::runtime_error("Scenario file must contain a non-empty 'scenarios' array");
    }

    std::vector<NamedScenario> result;
    result.reserve(static_cast<std::size_t>(scenarios.size()));
    for (qsizetype index = 0; index < scenarios.size(); ++index) {
        if (!scenarios.at(index).isObject()) {
            throw std::runtime_error("Each scenario must be a JSON object");
        }
        const QJsonObject object = scenarios.at(index).toObject();
        NamedScenario scenario;
        const QJsonValue name_value = object.value("name");
        if (!name_value.isUndefined() && !name_value.isString()) {
            throw std::runtime_error("Scenario name must be a string");
        }
        scenario.name = name_value.isUndefined()
            ? QStringLiteral("Scenario %1").arg(static_cast<qlonglong>(index + 1))
            : name_value.toString();

        const auto read_unsigned = [&object](const char* key, std::uint64_t fallback,
                                             std::uint64_t minimum, std::uint64_t maximum) {
            const QJsonValue value = object.value(QLatin1String(key));
            if (value.isUndefined()) {
                return fallback;
            }
            if (!value.isDouble()) {
                throw std::runtime_error(std::string("Scenario field '") + key + "' must be an integer");
            }
            const double number = value.toDouble();
            if (!std::isfinite(number) || std::floor(number) != number
                || number < static_cast<double>(minimum) || number > static_cast<double>(maximum)) {
                throw std::runtime_error(std::string("Scenario field '") + key + "' is outside its allowed range");
            }
            return static_cast<std::uint64_t>(number);
        };
        const auto read_probability = [&object](const char* key, double fallback) {
            const QJsonValue value = object.value(QLatin1String(key));
            if (value.isUndefined()) {
                return fallback;
            }
            if (!value.isDouble()) {
                throw std::runtime_error(std::string("Scenario field '") + key + "' must be numeric");
            }
            return value.toDouble();
        };

        scenario.config.approach_length = static_cast<std::uint32_t>(
            read_unsigned("approach_length", scenario.config.approach_length, 2, 500));
        scenario.config.steps = read_unsigned("steps", scenario.config.steps, 1, 10000000);
        scenario.config.arrival_rate = read_probability("arrival_rate", scenario.config.arrival_rate);
        scenario.config.initial_density = read_probability("initial_density", scenario.config.initial_density);
        scenario.config.seed = read_unsigned("seed", scenario.config.seed, 0, 9007199254740991ULL);
        scenario.config.north_south_green_ticks = static_cast<std::uint32_t>(
            read_unsigned("north_south_green_ticks", scenario.config.north_south_green_ticks, 1, 1000000));
        scenario.config.east_west_green_ticks = static_cast<std::uint32_t>(
            read_unsigned("east_west_green_ticks", scenario.config.east_west_green_ticks, 1, 1000000));

        const std::string validation_error = validate_config(scenario.config);
        if (!validation_error.empty()) {
            throw std::runtime_error(
                "Invalid scenario '" + scenario.name.toStdString() + "': " + validation_error);
        }
        result.push_back(scenario);
    }
    return result;
}

} // namespace traffic
