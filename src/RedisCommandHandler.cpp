#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include "../include/RespParser.h"

#include <cctype>

RedisCommandHandler::RedisCommandHandler(RedisDatabase& database) : database_(&database) {}

std::string RedisCommandHandler::processCommand(const std::string& commandLine) {
    std::vector<std::string> tokens;
    std::size_t consumed = 0;
    const ParseResult result = RespParser::tryParseCommand(commandLine, tokens, consumed);
    if (result == ParseResult::Incomplete) return RespParser::error("incomplete request");
    if (result == ParseResult::Invalid) return RespParser::error("invalid request");
    return processCommand(tokens);
}

std::string RedisCommandHandler::processCommand(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return RespParser::error("empty command");

    std::string command = tokens[0];
    for (char& character : command) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }

    if (command == "PING") {
        if (tokens.size() != 1) return RespParser::error("wrong number of arguments");
        return RespParser::simpleString("PONG");
    }
    if (command == "SET") {
        if (tokens.size() != 3) return RespParser::error("wrong number of arguments");
        database_->set(tokens[1], tokens[2]);
        return RespParser::simpleString("OK");
    }
    if (command == "GET") {
        if (tokens.size() != 2) return RespParser::error("wrong number of arguments");
        const auto value = database_->get(tokens[1]);
        return value ? RespParser::bulkString(*value) : RespParser::nullBulkString();
    }
    if (command == "DEL") {
        if (tokens.size() != 2) return RespParser::error("wrong number of arguments");
        return RespParser::integer(database_->del(tokens[1]) ? 1 : 0);
    }
    if (command == "EXISTS") {
        if (tokens.size() != 2) return RespParser::error("wrong number of arguments");
        return RespParser::integer(database_->exists(tokens[1]) ? 1 : 0);
    }
    return RespParser::error("unknown command '" + command + "'");
}
