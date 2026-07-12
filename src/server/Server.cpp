#include "Server.hpp"
#include "../common/common.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

namespace chat
{

    Server::Server(int port)
        : port_(port), server_fd_(-1), client_count_(0), running_(true)
    {
        client_fds_.fill(-1);
        logger_ = std::make_unique<Logger>("server.log");
        // Игнорируем SIGPIPE, чтобы не падать при отправке на закрытый сокет
        signal(SIGPIPE, SIG_IGN);
    }

    Server::~Server()
    {
        shutdown_server();
        if (accept_thread_.joinable())
            accept_thread_.join();
        for (auto &t : client_threads_)
        {
            if (t.joinable())
                t.join();
        }
    }

    bool Server::setup_server()
    {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0)
        {
            perror("socket");
            return false;
        }

        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("setsockopt");
            close(server_fd_);
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = INADDR_ANY;

        // // Преобразуем строку "192.168.1.15" в сетевой формат
        // if (inet_pton(AF_INET, SERVER_HOST, &addr.sin_addr) != 1)
        // {
        //     perror("inet_pton");
        //     return false;
        // }

        if (bind(server_fd_, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("bind");
            close(server_fd_);
            return false;
        }

        if (listen(server_fd_, 2) < 0)
        {
            perror("listen");
            close(server_fd_);
            return false;
        }

        logger_->log("Server started on port " + std::to_string(port_));
        std::cout << "Server listening on port " << port_ << std::endl;
        return true;
    }

    void Server::accept_clients()
    {
        while (running_ && client_count_ <= 2)
        {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);

            
            int client_fd = accept(server_fd_, (sockaddr *)&client_addr, &len);
            if (client_fd < 0)
            {
                if (!running_ || server_fd_ == -1)
                    break; 
                if (errno == EINTR || errno == EBADF)
                    continue;
                perror("accept");
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                if (client_count_ >= 2)
                {
                    send(client_fd, MSG_MAX_CLIENTS.c_str(), MSG_MAX_CLIENTS.size(), 0);
                    close(client_fd);
                    
                    logger_->log("Rejected extra connection from " + std::string(inet_ntoa(client_addr.sin_addr)));
                    continue;
                }

                int id = client_count_.fetch_add(1);
                client_fds_[id] = client_fd;
                logger_->log("Client #" + std::to_string(id) + " connected from " + inet_ntoa(client_addr.sin_addr));
                std::cout << "Client #" << id << " connected (fd=" << client_fd << ")" << std::endl;

                client_threads_[id] = std::thread(&Server::client_handler, this, client_fd, id);

                if (client_count_ == 2)
                {
                    std::cout << "Both clients connected. Chat started!" << std::endl;
                    logger_->log("Both clients connected. Chat started.");
                    broadcast(MSG_CHAT_STARTED + "\n", -1);
                }
            }
        }
        
    }

    void Server::client_handler(int client_fd, int client_id)
    {
        char buffer[1024];
        while (running_)
        {
            ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0)
            {
                {
                    std::lock_guard<std::mutex> lock(clients_mutex_);
                    if (client_fds_[client_id] != -1)
                    {
                        close(client_fds_[client_id]);
                        client_fds_[client_id] = -1;
                        logger_->log("Client #" + std::to_string(client_id) + " disconnected.");
                        std::cout << "Client #" << client_id << " disconnected." << std::endl;

                        int other_id = (client_id == 0) ? 1 : 0;
                        if (client_fds_[other_id] != -1)
                        {
                            send(client_fds_[other_id], MSG_OTHER_DISCONNECTED.c_str(),
                                 MSG_OTHER_DISCONNECTED.size(), 0);
                        }
                    }
                }
                shutdown_server();
                client_fd = -1; 
                break;
            }

            buffer[n] = '\0';
            std::string message(buffer, n);
            std::string cmd = message;
            if (!cmd.empty() && cmd.back() == DELIMITER)
                cmd.pop_back();

            if (cmd == CMD_SHUTDOWN)
            {
                logger_->log("Client #" + std::to_string(client_id) + " issued SHUTDOWN.");
                std::cout << "Shutdown command received from client #" << client_id << std::endl;
                broadcast(MSG_SERVER_SHUTDOWN + "\n", -1);
                shutdown_server();
                client_fd = -1;
                break;
            }

            int other_id = (client_id == 0) ? 1 : 0;
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                if (client_fds_[other_id] != -1)
                {
                    ssize_t sent = send(client_fds_[other_id], message.c_str(), message.size(), 0);
                    std::cout << "Relayed to client #" << other_id << ": " << message;
                    // ... логирование
                }
                else
                {
                    logger_->log("Other client disconnected, closing connection for #" + std::to_string(client_id));
                    send(client_fd, MSG_OTHER_DISCONNECTED.c_str(), MSG_OTHER_DISCONNECTED.size(), 0);
                    // Здесь не нужно закрывать сокет, пусть цикл завершится по recv или по shutdown_server
                }
            }
        }

        if (client_fd != -1)
        {
            close(client_fd);
        }
    }

    void Server::broadcast(const std::string &message, int exclude_id)
    {
        // std::lock_guard<std::mutex> lock(clients_mutex_);   // включить синхронизацию
        for (int i = 0; i < 2; ++i)
        {
            if (i == exclude_id)
                continue;
            if (client_fds_[i] != -1)
            {
                send(client_fds_[i], message.c_str(), message.size(), 0);
            }
        }
    }

    void Server::shutdown_server()
    {

        if (!running_)
            return;
        running_ = false;

        std::lock_guard<std::mutex> lock(clients_mutex_);

        for (int i = 0; i < 2; ++i)
        {
            if (client_fds_[i] != -1)
            {
                close(client_fds_[i]);
                client_fds_[i] = -1;
            }
        }
        if (server_fd_ != -1)
        {
            shutdown(server_fd_, SHUT_RDWR);
            close(server_fd_);
            server_fd_ = -1;
        }
        logger_->log("Server shut down.");
        std::cout << "Server stopped." << std::endl;
    }

    void Server::run()
    {
        if (!setup_server())
            return;

        accept_thread_ = std::thread(&Server::accept_clients, this);
        if (accept_thread_.joinable())
            accept_thread_.join();

        for (auto &t : client_threads_)
        {
            if (t.joinable())
                t.join();
        }

        if (running_)
            shutdown_server();
    }

} // namespace chat