#include "pch.h"

#include "MemoryUtils.h"

#include "Logging.h"

ScopedVirtualProtect::ScopedVirtualProtect(void* address, size_t size, DWORD protection, bool flush_instruction_cache)
	: address_(address), size_(size), flush_instruction_cache_(flush_instruction_cache)
{
	if (!VirtualProtect(address, size, protection, &old_protection_))
	{
		LOG(LOG_ERROR) << "Failed to update protection at memory address " << HEX(address) << ". Error code: " << GetLastError();
		valid_ = false;
		return;
	}
	valid_ = true;
}

ScopedVirtualProtect::~ScopedVirtualProtect()
{
	DWORD unused = 0;
	VirtualProtect(address_, size_, old_protection_, &unused);
	if (flush_instruction_cache_)
	{
		FlushInstructionCache(GetCurrentProcess(), address_, size_);
	}
}

bool WriteJmp(BYTE* address, const void* target, DWORD byte_count)
{
	if (byte_count < 5)
	{
		LOG(LOG_ERROR) << "WriteJmp() requires at least 5 bytes";
		return false;
	}
	ScopedVirtualProtect protect(address, byte_count, PAGE_READWRITE);
	if (!protect.valid()) return false;

	*address = 0xE9;  // jmp (near)
	*(DWORD*)(address + 1) = (DWORD)target - (DWORD)address - 5;

	// NOP remaining bytes
	for (DWORD x = 5; x < byte_count; ++x)
	{
		*(address + x) = 0x90;
	}
	return true;
}
