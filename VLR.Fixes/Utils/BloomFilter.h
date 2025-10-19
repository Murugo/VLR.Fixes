#pragma once

#include <filesystem>
#include <string>
#include <vector>

class BloomFilter
{
public:
    BloomFilter(int expected_size, double p_false_positive);

    void Insert(const std::string& value);
    void Insert(const std::filesystem::path& path);

    bool Exists(const std::string& value) const;
    bool Exists(const std::filesystem::path& path) const;

    bool Empty() const { return filter_.empty(); }

private:
    std::vector<bool> filter_;
    int hash_count_;
};
