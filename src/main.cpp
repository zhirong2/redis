#include "../include/RedisServer.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    int port = 6379;

    try {
        if (argc >= 2) port = std::stoi(argv[1]);
    } catch (const std::exception&) {
        std::cerr << "Usage: " << argv[0] << " [port]\n";
        return 1;
    }
    if (port < 1 || port > 65535) {
        std::cerr << "Port must be between 1 and 65535\n";
        return 1;
    }

    RedisServer server(port);

    server.run();
    return 0;
}
