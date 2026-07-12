#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace chat {

Logger::Logger(const std::string& filename) {
    // Открываем файл в режиме добавления (app): новые записи дописываются в конец, старые не стираются.
    file_.open(filename, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        // Если файл не удалось открыть — выводим предупреждение в stderr.
        // В продакшене тут может быть более сложная обработка (исключение, fallback‑лог и т.п.).
        std::cerr << "Warning: Could not open log file " << filename << std::endl;
    }
}

Logger::~Logger() {
    // Явно закрываем файл, если он открыт.
    // std::ofstream закроет его и сам в деструкторе, но явный close() делает намерение кода понятным.
    if (file_.is_open()) file_.close();
}

void Logger::log(const std::string& message) {
    // Потокобезопасная блокировка: пока этот блок активен, другие потоки не смогут войти в log().
    std::lock_guard<std::mutex> lock(mtx_);

    // Если файл не открыт (например, ошибка в конструкторе), просто выходим.
    if (!file_.is_open()) return;

    // Получаем текущее время
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&time_t);

    // Записываем строку вида: "2025-12-10 14:35:20 Сообщение"
    file_ << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << " " << message << std::endl;

    // Принудительно сбрасываем буфер на диск.
    // Без flush() часть данных может оставаться в буфере и быть потеряна при аварийном завершении.
    file_.flush();
}

} // namespace chat