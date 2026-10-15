#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

class SpeedMeter {
public:
    using Clock = std::chrono::steady_clock;

    explicit SpeedMeter(Clock::time_point start = Clock::now());

    void add(std::uint64_t bytes);
    std::uint64_t instant();
    double average() const;
    std::uint64_t total() const;

private:
    mutable std::mutex mu_;
    Clock::time_point start_;
    Clock::time_point last_;
    std::uint64_t total_ = 0;
    std::uint64_t last_total_ = 0;
};

std::string human_rate(std::uint64_t bps);
std::string human_size(std::uint64_t bytes);