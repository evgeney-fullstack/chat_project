#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace chat {

Logger::Logger(const std::string& filename) {
    file_.open(filename, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        std::cerr << "Warning: Could not open log file " << filename << std::endl;
    }
}

Logger::~Logger() {
    if (file_.is_open()) file_.close();
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!file_.is_open()) return;

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&time_t);
    file_ << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << " " << message << std::endl;
    file_.flush();
}

} // namespace chat