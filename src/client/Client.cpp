#include "Client.hpp"
#include "../common/common.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>

namespace chat {

Client::Client(const std::string& host, int port)
    : host_(host), port_(port), sock_fd_(-1), connected_(false),
      sent_count_(0), received_count_(0), prompt_needed_(true), chat_started(false)
{
    start_time_ = std::chrono::steady_clock::now();
    signal(SIGPIPE, SIG_IGN);
}

Client::~Client() {
    shutdown();
}

bool Client::connect_to_server() {
    sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd_ < 0) {
        perror("socket");
        return false;
    }

    struct hostent* server = gethostbyname(host_.c_str());
    if (server == nullptr) {
        std::cerr << "No such host: " << host_ << std::endl;
        close(sock_fd_);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(port_);

    if (connect(sock_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock_fd_);
        return false;
    }

    std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
    connected_ = true;
    return true;
}

bool Client::is_system_message(const std::string& message) {
    if (message.find(MSG_OTHER_DISCONNECTED) != std::string::npos) {
        std::cout << "\n[System] " << MSG_OTHER_DISCONNECTED << std::endl;
        connected_ = false;
        return true;
    }
    if (message.find(MSG_SERVER_SHUTDOWN) != std::string::npos) {
        std::cout << "\n[System] " << MSG_SERVER_SHUTDOWN << std::endl;
        connected_ = false;
        return true;
    }
    if (message.find(MSG_MAX_CLIENTS) != std::string::npos) {
        std::cout << "\n[System] " << MSG_MAX_CLIENTS << std::endl;
        connected_ = false;
        return true;
    }
    if (message.find(MSG_CHAT_STARTED) != std::string::npos) {
        std::cout << "\n[System] " << MSG_CHAT_STARTED << std::endl;
        chat_started = true;
        return true;
    }
    return false;
}

void Client::handle_server_message() {
    char buffer[1024];
    ssize_t n = recv(sock_fd_, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        if (n == 0)
            std::cout << "\n[System] " << MSG_SERVER_DISCONNECT << std::endl;
        else
            perror("recv");
        connected_ = false;
        return;
    }
    buffer[n] = '\0';
    std::string message(buffer, n);

    if (is_system_message(message)) {
        if (connected_) // если не отключились, нужно показать приглашение
            prompt_needed_ = true;
        return;
    }

    // Обычное сообщение от собеседника
    if (!message.empty() && message.back() == DELIMITER)
        message.pop_back();
    std::cout << "\n[Friend] " << message << std::endl;
    received_count_++;
    prompt_needed_ = true;
}

void Client::process_command(const std::string& line) {
    if (line == CMD_EXIT) {
        std::cout << "Exiting..." << std::endl;
        connected_ = false;
        return;
    }
    if (line == CMD_STATS) {
        print_stats();
        prompt_needed_ = true;
        return;
    }

    // Отправка сообщения
    std::string to_send = line + DELIMITER;
    ssize_t sent = send(sock_fd_, to_send.c_str(), to_send.size(), 0);
    if (sent < 0) {
        perror("send");
        connected_ = false;
    } else {
        sent_count_++;
        prompt_needed_ = true;
    }
}

void Client::handle_user_input() {
    char ch;
    while (true) {
        ssize_t bytes = read(STDIN_FILENO, &ch, 1);
        if (bytes <= 0) {
            if (bytes == 0 || errno != EAGAIN) {
                connected_ = false; // EOF или ошибка
            }
            break;
        }
        if (ch == '\n' || ch == '\r') {
            if (!input_buffer_.empty()) {
                std::string line = input_buffer_;
                input_buffer_.clear();
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                process_command(line);
                // После обработки команды выходим, чтобы обновить приглашение в цикле
                break;
            }
            // Пустая строка – просто выходим
            break;
        } else {
            input_buffer_.push_back(ch);
        }
    }
}

void Client::print_stats() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    std::cout << "\n===== STATISTICS =====" << std::endl;
    std::cout << "Connected for: " << duration.count() << " seconds" << std::endl;
    std::cout << "Messages sent: " << sent_count_.load() << std::endl;
    std::cout << "Messages received: " << received_count_.load() << std::endl;
    std::cout << "======================" << std::endl;
    std::cout.flush();
}

void Client::shutdown() {
    connected_ = false;
    if (sock_fd_ != -1) {
        close(sock_fd_);
        sock_fd_ = -1;
    }
}

void Client::run() {
    if (!connect_to_server())
        return;

    // Устанавливаем неблокирующий режим для stdin
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get");
        return;
    }
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl set nonblock");
        return;
    }

    fd_set readfds;
    while (connected_) {
        if (prompt_needed_ && chat_started) {
            std::cout << "You: " << std::flush;
            prompt_needed_ = false;
        }

        FD_ZERO(&readfds);
        FD_SET(sock_fd_, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int max_fd = std::max(sock_fd_, STDIN_FILENO);

        struct timeval tv = {0, 100000}; // 100 мс
        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, &tv);

        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        if (FD_ISSET(sock_fd_, &readfds))
            handle_server_message();

        if (FD_ISSET(STDIN_FILENO, &readfds))
            handle_user_input();
    }

    // Восстанавливаем блокирующий режим для stdin
    fcntl(STDIN_FILENO, F_SETFL, flags);
    shutdown();
    std::cout << "Client stopped." << std::endl;
}

} // namespace chat