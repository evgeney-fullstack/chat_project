#pragma once

#include <string>
#include <array>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include "Logger.hpp"

namespace chat {

class Server {
public:
    Server(int port);          // Конструктор: инициализирует сервер на указанном порту
    ~Server();                 // Деструктор: корректно останавливает сервер и ждёт завершения потоков

    void run();                // Основной метод запуска: настройка + цикл работы сервера

private:
    // Вспомогательные методы (реализация скрыта внутри класса)
    bool setup_server();                    // Создаёт и настраивает слушающий сокет (socket, bind, listen)
    void accept_clients();                  // Поток: принимает входящие соединения и запускает обработчики
    void client_handler(int client_fd, int client_id); // Обрабатывает сообщения конкретного клиента
    void broadcast(const std::string& message, int exclude_id = -1); // Рассылка сообщения всем (кроме одного) клиентам
    void shutdown_server();                 // Корректная остановка: закрытие сокетов, флагов, логгирование

    // Поля данных
    int port_;                              // Порт, на котором слушает сервер
    int server_fd_;                         // Файловый дескриптор слушающего сокета (-1, если не создан)
    std::array<int, 2> client_fds_;         // Дескрипторы подключённых клиентов (максимум 2)
    std::atomic<int> client_count_;          // Атомарный счётчик подключённых клиентов
    std::mutex clients_mutex_;              // Мьютекс для защиты общих структур при одновременной работе потоков
    std::atomic<bool> running_;             // Флаг: работает ли сервер (атомарный, чтобы безопасно читать из разных потоков)

    std::unique_ptr<Logger> logger_;        // Логгер: RAII‑управление временем жизни (автоматически удалится в деструкторе)

    std::thread accept_thread_;            // Поток, который принимает новые соединения
    std::array<std::thread, 2> client_threads_; // Потоки для обработки каждого из двух клиентов
};

} // namespace chat