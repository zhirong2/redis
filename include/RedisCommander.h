#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>

class RedisCommanderHandler {
public:
    RedisCommanderHandler();

    std::string processCommand(const std::string& commandLine);

};

#endif