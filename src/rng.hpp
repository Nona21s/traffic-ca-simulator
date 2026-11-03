#pragma once

#include <cstdint>

namespace traffic {
namespace rng {

inline std::uint64_t mix(std::uint64_t value) noexcept {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

inline double unit_interval(std::uint64_t seed, std::uint64_t key) noexcept {
    const std::uint64_t bits = mix(seed ^ mix(key));
    return static_cast<double>(bits >> 11U) * 0x1.0p-53;
}

inline bool event(std::uint64_t seed, std::uint64_t key, double probability) noexcept {
    return unit_interval(seed, key) < probability;
}

} // namespace rng
} // namespace traffic
