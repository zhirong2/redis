#include "../include/RedisDatabase.h"

void RedisDatabase::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[key] = value;
}

std::optional<std::string> RedisDatabase::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = values_.find(key);
    if (iterator == values_.end()) return std::nullopt;
    return iterator->second;
}

bool RedisDatabase::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return values_.erase(key) != 0;
}

bool RedisDatabase::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return values_.find(key) != values_.end();
}
