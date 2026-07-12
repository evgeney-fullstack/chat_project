#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace chat {

class Logger {
public:
    explicit Logger(const std::string& filename);
    // Конструктор: открывает файл для записи.
    // explicit запрещает неявные преобразования (например, нельзя передать строку там, где ожидается Logger).

    ~Logger();
    // Деструктор: закрывает файл (ofstream делает это автоматически, но явный деструктор полезен для чистоты API).

    void log(const std::string& message);
    // Записывает сообщение в лог с потокобезопасной блокировкой.

private:
    std::ofstream file_;
    // Поток для записи в файл.

    std::mutex mtx_;
    // Мьютекс для синхронизации: если log() вызовут из нескольких потоков одновременно,
    // только один поток будет писать в файл в конкретный момент.
};

} // namespace chat