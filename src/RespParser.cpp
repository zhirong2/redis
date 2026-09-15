#include "../include/RespParser.h"

#include <limits>
#include <sstream>

namespace {
ParseResult parseLength(const std::string& buffer, std::size_t start, long long& length, std::size_t& next) {
    const std::size_t end = buffer.find("\r\n", start);
    if (end == std::string::npos) return ParseResult::Incomplete;
    if (end == start) return ParseResult::Invalid;

    std::size_t position = start;
    bool negative = false;
    if (buffer[position] == '-') {
        negative = true;
        ++position;
    }
    if (position == end) return ParseResult::Invalid;

    long long value = 0;
    for (; position < end; ++position) {
        const char character = buffer[position];
        if (character < '0' || character > '9') return ParseResult::Invalid;
        const int digit = character - '0';
        if (value > (std::numeric_limits<long long>::max() - digit) / 10) {
            return ParseResult::Invalid;
        }
        value = value * 10 + digit;
    }
    length = negative ? -value : value;
    next = end + 2;
    return ParseResult::Complete;
}
}

ParseResult RespParser::tryParseCommand(const std::string& buffer,
                                        std::vector<std::string>& command,
                                        std::size_t& consumed) {
    command.clear();
    consumed = 0;
    if (buffer.empty()) return ParseResult::Incomplete;
    if (buffer[0] != '*') return ParseResult::Invalid;

    long long elementCount = 0;
    std::size_t position = 0;
    ParseResult result = parseLength(buffer, 1, elementCount, position);
    if (result != ParseResult::Complete) return result;
    if (elementCount < 0 || elementCount > 1024) return ParseResult::Invalid;

    for (long long index = 0; index < elementCount; ++index) {
        if (position >= buffer.size()) return ParseResult::Incomplete;
        if (buffer[position] != '$') return ParseResult::Invalid;

        long long valueLength = 0;
        result = parseLength(buffer, position + 1, valueLength, position);
        if (result != ParseResult::Complete) return result;
        if (valueLength < 0 || valueLength > static_cast<long long>(buffer.size())) {
            return valueLength < 0 ? ParseResult::Invalid : ParseResult::Incomplete;
        }
        const std::size_t length = static_cast<std::size_t>(valueLength);
        if (buffer.size() - position < length + 2) return ParseResult::Incomplete;
        if (buffer[position + length] != '\r' || buffer[position + length + 1] != '\n') {
            return ParseResult::Invalid;
        }
        command.push_back(buffer.substr(position, length));
        position += length + 2;
    }

    consumed = position;
    return ParseResult::Complete;
}

std::string RespParser::simpleString(const std::string& value) {
    return "+" + value + "\r\n";
}

std::string RespParser::error(const std::string& message) {
    return "-ERR " + message + "\r\n";
}

std::string RespParser::integer(long long value) {
    return ":" + std::to_string(value) + "\r\n";
}

std::string RespParser::bulkString(const std::string& value) {
    return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
}

std::string RespParser::nullBulkString() {
    return "$-1\r\n";
}
