#pragma once

#include "pch.h"

#define RETURN_IF_PATTERN_NOT_FOUND(pattern) \
if (pattern.size() != 1) \
{ \
  LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for pattern " << #pattern; \
  return false; \
}

// Sets the protection of the given memory region while this object is in
// scope. Resets the protection on destruction.
class ScopedVirtualProtect {
public:
    ScopedVirtualProtect(void* address, size_t size, DWORD protection, bool flush_instruction_cache = true);
    ~ScopedVirtualProtect();

    // Returns whether changing the protection of the memory region succeeded.
    bool valid() const { return valid_; }

    ScopedVirtualProtect(const ScopedVirtualProtect&) = delete;
    ScopedVirtualProtect& operator=(const ScopedVirtualProtect&) = delete;

private:
    void* address_;
    size_t size_;
    bool flush_instruction_cache_;
    DWORD old_protection_;
    bool valid_;
};

// Writes a near `jmp` instruction at the given address.
bool WriteJmp(BYTE* address, const void* target, DWORD byte_count);
