#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include "../include/RespParser.h"

#include <cerrno>
#include <csignal>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

static RedisServer* globalServer = nullptr;

namespace {
void handleSignal(int) {
    if (globalServer != nullptr) globalServer->shutdown();
}
}

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true) {
    globalServer = this;
}

void RedisServer::shutdown() {
    const bool wasRunning = running.exchange(false);
    if (wasRunning && server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
}

bool RedisServer::sendAll(int socket, const std::string& response) {
    std::size_t sentTotal = 0;
    while (sentTotal < response.size()) {
        const ssize_t sent = send(socket, response.data() + sentTotal,
                                  response.size() - sentTotal, 0);
        if (sent > 0) {
            sentTotal += static_cast<std::size_t>(sent);
            continue;
        }
        if (sent < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

void RedisServer::handleClient(int client_socket, RedisCommandHandler& command_handler) {
    std::string pending;
    char buffer[4096];

    while (true) {
        const ssize_t bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes == 0) break;
        if (bytes < 0) {
            if (errno == EINTR) continue;
            break;
        }
        pending.append(buffer, static_cast<std::size_t>(bytes));

        while (!pending.empty()) {
            std::vector<std::string> command;
            std::size_t consumed = 0;
            const ParseResult result = RespParser::tryParseCommand(pending, command, consumed);
            if (result == ParseResult::Incomplete) break;
            if (result == ParseResult::Invalid) {
                sendAll(client_socket, RespParser::error("invalid request"));
                close(client_socket);
                return;
            }
            if (!sendAll(client_socket, command_handler.processCommand(command))) {
                close(client_socket);
                return;
            }
            pending.erase(0, consumed);
        }
    }

    close(client_socket);
}

void RedisServer::run() {
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating server socket\n";
        return;
    }

    int option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(static_cast<uint16_t>(port));
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        std::cerr << "Error binding server socket\n";
        close(server_socket);
        server_socket = -1;
        return;
    }
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error listening on server socket\n";
        close(server_socket);
        server_socket = -1;
        return;
    }

    std::cout << "Redis server listening on port " << port << "\n";
    RedisDatabase database;
    RedisCommandHandler command_handler(database);
    std::vector<std::thread> threads;

    while (running) {
        const int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            if (running && errno != EINTR) std::cerr << "Error accepting client connection\n";
            continue;
        }
        threads.emplace_back(&RedisServer::handleClient, this, client_socket,
                             std::ref(command_handler));
    }

    for (std::thread& thread : threads) {
        if (thread.joinable()) thread.join();
    }
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
}
