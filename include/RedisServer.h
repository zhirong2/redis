#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include<string>
#include<atomic>
#include<vector>

class RedisCommandHandler;


class RedisServer {
public:
    RedisServer(int port);

    void run();
    void shutdown();

private:
    void handleClient(int client_socket, RedisCommandHandler& command_handler);
    static bool sendAll(int socket, const std::string& response);

    int port;
    int server_socket;
    std::atomic<bool> running;
};

#endif
