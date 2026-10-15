#include "../include/speed_meter.h"
#include <cstdio>

SpeedMeter::SpeedMeter(Clock::time_point start): start_(start), last_(start) {}

void SpeedMeter::add(std::uint64_t bytes) {
    std::lock_guard<std::mutex> lk(mu_);
    total_ += bytes;
}

std::uint64_t SpeedMeter::instant() {
    std::lock_guard<std::mutex> lk(mu_);
    auto now = Clock::now();
    double dt = std::chrono::duration<double>(now - last_).count();
    std::uint64_t delta = total_ - last_total_;
    last_ = now;
    last_total_ = total_;
    if (dt <= 0.0) return 0;
    return static_cast<std::uint64_t>(delta / dt);
}

double SpeedMeter::average() const {
    std::lock_guard<std::mutex> lk(mu_);
    auto now = Clock::now();
    double dt = std::chrono::duration<double>(now - start_).count();
    if (dt <= 0.0) return 0.0;
    return static_cast<double>(total_) / dt;
}

std::uint64_t SpeedMeter::total() const {
    std::lock_guard<std::mutex> lk(mu_);
    return total_;
}

std::string human_rate(std::uint64_t bps) {
    const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    double v = static_cast<double>(bps);
    int u = 0;
    while (v >= 1000.0 && u < 3) {
        v /= 1000.0;
        ++u;
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), u == 0 ? "%.0f %s" : "%.2f %s", v, units[u]);
    return buf;
}

std::string human_size(std::uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1000.0 && u < 4) {
        v /= 1000.0;
        ++u;
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), u == 0 ? "%.0f %s" : "%.2f %s", v, units[u]);
    return buf;
}