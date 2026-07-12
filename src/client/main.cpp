#include "Client.hpp"
#include "../common/common.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::string host = "192.168.1.104";
    int port = chat::DEFAULT_PORT;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) {
        port = std::atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port. Using default " << chat::DEFAULT_PORT << std::endl;
            port = chat::DEFAULT_PORT;
        }
    }

    try {
        chat::Client client(host, port);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}