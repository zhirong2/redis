#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>
#include <vector>

class RedisDatabase;

class RedisCommandHandler {
public:
    explicit RedisCommandHandler(RedisDatabase& database);

    std::string processCommand(const std::string& commandLine);
    std::string processCommand(const std::vector<std::string>& tokens);

private:
    RedisDatabase* database_;
};

#endif
