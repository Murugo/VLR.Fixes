#include "pch.h"

#include "LuaHook.h"

#include <filesystem>

#include "Logging.h"
#include "MemoryUtils.h"
#include "..\External\Hooking.Patterns\Hooking.Patterns.h"

namespace vlr {

using luaL_loadbuffer_func = int(*)(void*, const char*, size_t, char*);
using lua_pcall_func = int(*)(void*, int, int, int);
using lua_tolstring_func = char* (*)(void*, int, size_t*);

luaL_loadbuffer_func luaL_loadbuffer = nullptr;
lua_pcall_func lua_pcall = nullptr;
lua_tolstring_func lua_tolstring = nullptr;
void** LuaStatePtr = nullptr;

static BYTE* jmpLoadTestScriptReturnAddr = nullptr;

constexpr unsigned char kLuaTestCompiled[] = {
        0x1b, 0x4c, 0x75, 0x61, 0x51, 0x0, 0x1, 0x4, 0x4, 0x4, 0x8, 0x0, 0x22, 0x0, 0x0, 0x0, 0x40, 0x73, 0x63, 0x72, 0x69,
        0x70, 0x74, 0x73, 0x2e, 0x6d, 0x6f, 0x64, 0x2e, 0x6c, 0x75, 0x61, 0x64, 0x65, 0x63, 0x5c, 0x54, 0x45, 0x53, 0x54, 0x5c,
        0x74, 0x65, 0x73, 0x74, 0x2e, 0x6c, 0x75, 0x61, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2,
        0x2, 0x4, 0x0, 0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0x41, 0x40, 0x0, 0x0, 0x1e, 0x40, 0x0, 0x1, 0x20, 0x0, 0x80,
        0x0, 0x2, 0x0, 0x0, 0x0, 0x4, 0x7, 0x0, 0x0, 0x0, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x66, 0x0, 0x4, 0x32, 0x0,
        0x0, 0x0, 0x56, 0x4c, 0x52, 0x2e, 0x46, 0x69, 0x78, 0x65, 0x73, 0x20, 0x2d, 0x2d, 0x2d, 0x20, 0x4c, 0x6f, 0x61, 0x64,
        0x65, 0x64, 0x20, 0x63, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x64, 0x20, 0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x20, 0x73,
        0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x66, 0x75, 0x6c, 0x6c, 0x79, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0,
        0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
};

constexpr int kLuaOk = 0;

void LoadTestScript()
{
    void* LuaState = *LuaStatePtr;
    if (luaL_loadbuffer(LuaState, reinterpret_cast<const char*>(kLuaTestCompiled), sizeof(kLuaTestCompiled), /*name=*/nullptr) != kLuaOk)
    {
        LOG(LOG_ERROR) << "Failed to load test script: " << lua_tolstring(LuaState, -1, /*len=*/nullptr);
        return;
    }
    if (lua_pcall(LuaState, /*nargs=*/0, /*nresults=*/0, /*errfunc=*/0) != kLuaOk)
    {
        LOG(LOG_ERROR) << "Failed to execute test script: " << lua_tolstring(LuaState, -1, /*len=*/nullptr);
        return;
    }
}

__declspec(naked) void __stdcall LoadTestScriptASM()
{
    __asm
    {
        push ecx
        call LoadTestScript
        pop ecx
        mov ebx, ecx
        mov [ebp - 0x24], ebx
        jmp jmpLoadTestScriptReturnAddr
    }
}

bool PatchCustomLuaScripts()
{
    auto lua_state_pattern = hook::pattern("8B 85 E4 FE FF FF 85 C0 74 09 50");
    if (lua_state_pattern.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for lua state";
        return false;
    }
    BYTE* lua_state_addr = lua_state_pattern.count(1).get(0).get<BYTE>(0) + 0x1A;
    LuaStatePtr = (void**)(*(DWORD*)lua_state_addr);
    lua_pcall = (lua_pcall_func)(*(DWORD*)(lua_state_addr + 0x1B) + (DWORD)lua_state_addr + 0x1F);
    lua_tolstring = (lua_tolstring_func)(*(DWORD*)(lua_state_addr + 0x35) + (DWORD)lua_state_addr + 0x3F);

    auto lua_loadbuffer_pattern = hook::pattern("8A 01 41 84 C0 75 F9 68");
    if (lua_loadbuffer_pattern.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for lua loadbuffer";
        return false;
    }
    BYTE* lua_loadbuffer_addr = lua_loadbuffer_pattern.count(1).get(0).get<BYTE>(0) + 0x18;
    luaL_loadbuffer = (luaL_loadbuffer_func)(*(DWORD*)(lua_loadbuffer_addr) + (DWORD)lua_loadbuffer_addr + 0x04);

    auto load_test_script_pattern_new = hook::pattern("8B D9 89 5D DC 8B 0D");
    if (load_test_script_pattern_new.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for test lua injection";
        return false;
    }
    BYTE* load_test_script_inject_addr = load_test_script_pattern_new.count(1).get(0).get<BYTE>(0);
    jmpLoadTestScriptReturnAddr = load_test_script_inject_addr + 0x05;

    LOG(LOG_INFO) << "Patching test Lua script...";
    WriteJmp(load_test_script_inject_addr, LoadTestScriptASM, 0x05);
    return true;
}

}  // namespace vlr
