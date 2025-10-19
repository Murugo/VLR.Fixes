#include "BloomFilter.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "MurmurHash3.h"

BloomFilter::BloomFilter(int expected_size, double p_false_positive)
{
    if (expected_size <= 0 || p_false_positive <= 0.0)
    {
        hash_count_ = 0;
        return;
    }

    // Filter size in bits: m = -(n * ln(p_fp)) / ln(2)^2
    size_t size = static_cast<int>(-(expected_size * log(p_false_positive)) / pow(log(2), 2));
    filter_.resize(size);

    // Hash function count: k = m / n * ln(2)
    hash_count_ = static_cast<int>(static_cast<double>(size) / expected_size * log(2));
}

void BloomFilter::Insert(const std::string& value)
{
    if (filter_.empty()) return;

    for (int i = 0; i < hash_count_; ++i)
    {
        uint32_t hash = 0;
        MurmurHash3_x86_32(value.c_str(), static_cast<int>(value.size()), /*seed=*/i, &hash);
        filter_[hash % filter_.size()] = true;
    }
}

void BloomFilter::Insert(const std::filesystem::path& path)
{
    std::string key = path.string();
    std::transform(key.begin(), key.end(), key.begin(), tolower);
    Insert(key);
}

bool BloomFilter::Exists(const std::string& value) const
{
    if (filter_.empty()) return false;

    for (int i = 0; i < hash_count_; ++i)
    {
        uint32_t hash = 0;
        MurmurHash3_x86_32(value.c_str(), static_cast<int>(value.size()), /*seed=*/i, &hash);
        if (!filter_[hash % filter_.size()]) return false;
    }
    return true;
}

bool BloomFilter::Exists(const std::filesystem::path& path) const
{
    std::string key = path.string();
    std::transform(key.begin(), key.end(), key.begin(), tolower);
    return Exists(key);
}
