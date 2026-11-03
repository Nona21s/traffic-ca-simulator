#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace traffic {

enum class Approach : std::size_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

enum class SignalPhase {
    NorthSouth,
    EastWest
};

struct Vehicle {
    std::uint64_t waiting_ticks = 0;
};

using Lane = std::vector<std::optional<Vehicle>>;
using Lanes = std::array<Lane, 4>;

constexpr std::array<Approach, 4> all_approaches{
    Approach::North, Approach::East, Approach::South, Approach::West
};

constexpr std::size_t index_of(Approach approach) noexcept {
    return static_cast<std::size_t>(approach);
}

constexpr bool operator==(const Vehicle& left, const Vehicle& right) noexcept {
    return left.waiting_ticks == right.waiting_ticks;
}

constexpr bool operator!=(const Vehicle& left, const Vehicle& right) noexcept {
    return !(left == right);
}

} // namespace traffic
