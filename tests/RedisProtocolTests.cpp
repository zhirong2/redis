#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include "../include/RespParser.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static void testRequestParsing() {
    std::vector<std::string> command;
    std::size_t consumed = 0;

    const std::string request = "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$11\r\nhello world\r\n";
    assert(RespParser::tryParseCommand(request, command, consumed) == ParseResult::Complete);
    assert(command == std::vector<std::string>({"SET", "key", "hello world"}));
    assert(consumed == request.size());

    command.clear();
    consumed = 0;
    const std::string twoRequests = "*1\r\n$4\r\nPING\r\n*1\r\n$4\r\nPING\r\n";
    assert(RespParser::tryParseCommand(twoRequests, command, consumed) == ParseResult::Complete);
    assert(command == std::vector<std::string>({"PING"}));
    assert(consumed < twoRequests.size());

    command.clear();
    consumed = 0;
    assert(RespParser::tryParseCommand("*2\r\n$3\r\nGET\r\n$3\r\nke", command, consumed) == ParseResult::Incomplete);
    assert(RespParser::tryParseCommand("PING\r\n", command, consumed) == ParseResult::Invalid);
}

static void testResponseSerialization() {
    assert(RespParser::simpleString("PONG") == "+PONG\r\n");
    assert(RespParser::error("wrong number of arguments") == "-ERR wrong number of arguments\r\n");
    assert(RespParser::integer(1) == ":1\r\n");
    assert(RespParser::bulkString("hello") == "$5\r\nhello\r\n");
    assert(RespParser::nullBulkString() == "$-1\r\n");
}

static void testCommandBehavior() {
    RedisDatabase database;
    RedisCommandHandler handler(database);
    assert(handler.processCommand(std::vector<std::string>{"PING"}) == "+PONG\r\n");
    assert(handler.processCommand(std::vector<std::string>{"SET", "name", "Alice"}) == "+OK\r\n");
    assert(handler.processCommand(std::vector<std::string>{"GET", "name"}) == "$5\r\nAlice\r\n");
    assert(handler.processCommand(std::vector<std::string>{"EXISTS", "name"}) == ":1\r\n");
    assert(handler.processCommand(std::vector<std::string>{"DEL", "name"}) == ":1\r\n");
    assert(handler.processCommand(std::vector<std::string>{"GET", "name"}) == "$-1\r\n");
    assert(handler.processCommand(std::vector<std::string>{"NOPE"}) == "-ERR unknown command 'NOPE'\r\n");
    assert(handler.processCommand(std::vector<std::string>{"GET"}) == "-ERR wrong number of arguments\r\n");
}

int main() {
    testRequestParsing();
    testResponseSerialization();
    testCommandBehavior();
    std::cout << "Redis protocol tests passed\n";
}
