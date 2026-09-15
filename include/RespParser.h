#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include <cstddef>
#include <string>
#include <vector>

enum class ParseResult {
    Complete,
    Incomplete,
    Invalid
};

class RespParser {
public:
    static ParseResult tryParseCommand(const std::string& buffer,
                                       std::vector<std::string>& command,
                                       std::size_t& consumed);

    static std::string simpleString(const std::string& value);
    static std::string error(const std::string& message);
    static std::string integer(long long value);
    static std::string bulkString(const std::string& value);
    static std::string nullBulkString();
};

#endif
