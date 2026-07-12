#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace chat {

class Logger {
public:
    explicit Logger(const std::string& filename);
    ~Logger();

    void log(const std::string& message);

private:
    std::ofstream file_;
    std::mutex mtx_;
};

} // namespace chat