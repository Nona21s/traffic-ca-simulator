#pragma once

#include "config.hpp"
#include "rng.hpp"
#include "types.hpp"

#include <cstddef>
#include <cstdint>

namespace traffic {

class Grid {
public:
    Grid(std::uint32_t approach_length, double initial_density, std::uint64_t seed)
        : length_(approach_length) {
        for (std::size_t direction = 0; direction < lanes_.size(); ++direction) {
            lanes_[direction].resize(length_);
            for (std::size_t cell = 0; cell < length_; ++cell) {
                const std::uint64_t key = 0x100000000ULL
                    + static_cast<std::uint64_t>(direction) * length_ + cell;
                if (rng::event(seed, key, initial_density)) {
                    lanes_[direction][cell] = Vehicle{};
                }
            }
        }
    }

    const Lane& lane(Approach approach) const noexcept {
        return lanes_[index_of(approach)];
    }

    Lane& lane(Approach approach) noexcept {
        return lanes_[index_of(approach)];
    }

    std::uint32_t length() const noexcept {
        return length_;
    }

    void clear() noexcept {
        for (Lane& current_lane : lanes_) {
            for (auto& cell : current_lane) {
                cell.reset();
            }
        }
    }

    void swap(Grid& other) noexcept {
        lanes_.swap(other.lanes_);
    }

    std::size_t vehicle_count() const noexcept {
        std::size_t count = 0;
        for (const Lane& current_lane : lanes_) {
            for (const auto& cell : current_lane) {
                count += cell.has_value() ? 1U : 0U;
            }
        }
        return count;
    }

    bool same_state(const Grid& other) const noexcept {
        if (length_ != other.length_) {
            return false;
        }
        for (std::size_t direction = 0; direction < lanes_.size(); ++direction) {
            if (lanes_[direction] != other.lanes_[direction]) {
                return false;
            }
        }
        return true;
    }

private:
    std::uint32_t length_;
    Lanes lanes_;
};

} // namespace traffic
