#pragma once

#include <string>

namespace chat
{
    constexpr int DEFAULT_PORT = 1235;
    constexpr char DELIMITER = '\n';
    constexpr const char* SERVER_HOST = "192.168.1.104";


    const std::string MSG_MAX_CLIENTS = "ERROR: Server is full (max 2 clients).";
    const std::string MSG_OTHER_DISCONNECTED = "INFO: Other client disconnected. Chat ended.";
    const std::string MSG_SERVER_SHUTDOWN = "INFO: Server is shutting down.";
    const std::string MSG_CHAT_STARTED = "Chat started. You can send messages.";
    const std::string MSG_SERVER_DISCONNECT = "Server closed the connection.";

    const std::string CMD_SHUTDOWN = "SHUTDOWN";
    const std::string CMD_EXIT = "EXIT";
    const std::string CMD_STATS = "STATS";
}