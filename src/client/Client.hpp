#pragma once

#include <string>
#include <atomic>
#include <chrono>

namespace chat
{

    class Client
    {
    public:
        Client(const std::string &host, int port); // Конструктор: задаёт сервер и порт
        ~Client();                                 // Деструктор: корректно закрывает соединение

        void run(); // Основной цикл клиента: подключение + обработка ввода/вывода

    private:
        // Вспомогательные методы
        bool connect_to_server();                           // Устанавливает TCP‑соединение с сервером
        void handle_server_message();                       // Читает и обрабатывает сообщения от сервера
        void handle_user_input();                           // Обрабатывает ввод с клавиатуры (посимвольно)
        void process_command(const std::string &line);      // Выполняет команды (EXIT, STATS) или отправляет сообщение
        bool is_system_message(const std::string &message); // Проверяет, является ли сообщение системным уведомлением
        void print_stats() const;                           // Выводит статистику: время в чате, количество сообщений
        void shutdown();                                    // Закрывает сокет и сбрасывает состояние

        // Базовые поля
        int sock_fd_;      // Файловый дескриптор сокета (-1, если не подключён)
        std::string host_; // Адрес сервера
        int port_;         // Порт сервера
        bool chat_started; // Флаг: начался ли чат (получено MSG_CHAT_STARTED)

        // Потокобезопасные поля (атомарные)
        std::atomic<bool> connected_;                      // Статус подключения: атомарный, чтобы безопасно читать/писать из разных потоков
        std::chrono::steady_clock::time_point start_time_; // Момент старта соединения — нужен для подсчёта длительности
        std::atomic<size_t> sent_count_;                   // Количество отправленных сообщений (атомарно)
        std::atomic<size_t> received_count_;               // Количество полученных сообщений (атомарно)

        // Поля для неблокирующего ввода через select
        std::string input_buffer_; // Буфер накопления символов до нажатия Enter
        bool prompt_needed_;       // Флаг: нужно ли вывести приглашение «You: »
    };

} // namespace chat