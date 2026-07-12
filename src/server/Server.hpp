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
    Server(int port);
    ~Server();

    void run();

private:
    bool setup_server();
    void accept_clients();
    void client_handler(int client_fd, int client_id);
    void broadcast(const std::string& message, int exclude_id = -1);
    void shutdown_server();

    int port_;
    int server_fd_;
    std::array<int, 2> client_fds_;
    std::atomic<int> client_count_;
    std::mutex clients_mutex_;
    std::atomic<bool> running_;

    std::unique_ptr<Logger> logger_;

    std::thread accept_thread_;
    std::array<std::thread, 2> client_threads_;
};

} // namespace chat