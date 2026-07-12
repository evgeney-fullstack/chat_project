#include "Server.hpp"
#include "../common/common.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int port = chat::DEFAULT_PORT;
    if (argc >= 2) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port. Using default " << chat::DEFAULT_PORT << std::endl;
            port = chat::DEFAULT_PORT;
        }
    }

    try {
        chat::Server server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}