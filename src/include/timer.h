#pragma once

#include <cstdint>
#include <chrono>

#include "bus.h"

namespace Y86_64 {

// 1-byte auto-increment timer mapped at 0x4000. Advances at ~60Hz based on
// wall-clock time; natural uint8_t wraparound.
class Timer : public Device {
public:
    Timer() : last_tick_(Clock::now()) {}

    uint8_t read(uint64_t /*addr*/) override { return time_; }

    void write(uint64_t /*addr*/, uint8_t /*data*/) override {
        // Input-only; ignore writes.
    }

    // Call periodically; will increment enough steps to catch up to 60Hz.
    void update() {
        const auto now = Clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tick_);
        constexpr auto kStep = std::chrono::microseconds(16667); // ~60Hz
        while (elapsed >= kStep) {
            ++time_;
            last_tick_ += kStep;
            elapsed -= kStep;
        }
    }

private:
    using Clock = std::chrono::steady_clock;
    uint8_t time_{0};
    Clock::time_point last_tick_;
};

} // namespace Y86_64
