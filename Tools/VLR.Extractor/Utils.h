#pragma once

#include <cstdint>

uint32_t CalculateKey(const char* key);
void DecryptWithKey(uint32_t* dst, uint8_t* src, uint32_t size, uint32_t key, uint32_t unk);
