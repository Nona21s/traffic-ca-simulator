#pragma once

#include <cstdint>

namespace traffic {

class TrafficMetrics {
public:
    void record_departures(std::uint64_t count, std::uint64_t total_waiting_ticks) noexcept {
        departed_ += count;
        departed_waiting_ticks_ += total_waiting_ticks;
    }

    void record_queue(std::uint64_t queue_length) noexcept {
        queue_sum_ += queue_length;
        ++samples_;
    }

    std::uint64_t departed() const noexcept {
        return departed_;
    }

    double average_waiting_time() const noexcept {
        return departed_ == 0
            ? 0.0
            : static_cast<double>(departed_waiting_ticks_) / static_cast<double>(departed_);
    }

    double average_queue_length() const noexcept {
        return samples_ == 0
            ? 0.0
            : static_cast<double>(queue_sum_) / static_cast<double>(samples_);
    }

private:
    std::uint64_t departed_ = 0;
    std::uint64_t departed_waiting_ticks_ = 0;
    std::uint64_t queue_sum_ = 0;
    std::uint64_t samples_ = 0;
};

} // namespace traffic
