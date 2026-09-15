#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class RedisDatabase {
public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    bool exists(const std::string& key) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> values_;
};

#endif
