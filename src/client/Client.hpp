#pragma once

#include <string>
#include <atomic>
#include <chrono>

namespace chat {

class Client {
public:
    Client(const std::string& host, int port);
    ~Client();

    void run();

private:
    bool connect_to_server();
    void handle_server_message();
    void handle_user_input();
    void process_command(const std::string& line);
    bool is_system_message(const std::string& message);
    void print_stats() const;
    void shutdown();

    int sock_fd_;
    std::string host_;
    int port_;
    bool chat_started;

    std::atomic<bool> connected_;
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<size_t> sent_count_;
    std::atomic<size_t> received_count_;

    // Для неблокирующего ввода
    std::string input_buffer_;
    bool prompt_needed_;
};

} // namespace chat


