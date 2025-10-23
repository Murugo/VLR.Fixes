#include "pch.h"

#include "LuaHook.h"

#include <filesystem>

#include "Logging.h"
#include "MemoryUtils.h"
#include "resource.h"
#include "..\External\Hooking.Patterns\Hooking.Patterns.h"

namespace vlr {

using luaL_loadbuffer_func = int(*)(void*, const char*, size_t, char*);
using lua_pcall_func = int(*)(void*, int, int, int);
using lua_tolstring_func = char* (*)(void*, int, size_t*);

luaL_loadbuffer_func luaL_loadbuffer = nullptr;
lua_pcall_func lua_pcall = nullptr;
lua_tolstring_func lua_tolstring = nullptr;
void** LuaStatePtr = nullptr;

static BYTE* jmpGameStartHookReturnAddr = nullptr;

constexpr int kLuaOk = 0;

HMODULE GetCurrentModule()
{
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &hModule);
    return hModule;
}

void LoadSkipTransitionsScript()
{
    HMODULE module = GetCurrentModule();
    const HRSRC lua_resource = ::FindResource(module, MAKEINTRESOURCE(IDR_LUA_SKIP), RT_RCDATA);
    if (lua_resource == NULL)
    {
        LOG(LOG_ERROR) << "Failed to find Lua script resource. Error code: " << GetLastError();
        return;
    }
    const DWORD lua_file_size = ::SizeofResource(module, lua_resource);
    const HGLOBAL lua_resource_handle = ::LoadResource(module, lua_resource);
    if (lua_resource_handle == NULL)
    {
        LOG(LOG_ERROR) << "Failed to load Lua script resource. Error code: " << GetLastError();
        return;
    }
    const void* lua_file = ::LockResource(lua_resource_handle);
    
    void* LuaState = *LuaStatePtr;
    if (luaL_loadbuffer(LuaState, reinterpret_cast<const char*>(lua_file), lua_file_size, /*name=*/nullptr) != kLuaOk)
    {
        LOG(LOG_ERROR) << "Failed to load Lua script: " << lua_tolstring(LuaState, -1, /*len=*/nullptr);
        return;
    }
    if (lua_pcall(LuaState, /*nargs=*/0, /*nresults=*/0, /*errfunc=*/0) != kLuaOk)
    {
        LOG(LOG_ERROR) << "Failed to execute Lua script: " << lua_tolstring(LuaState, -1, /*len=*/nullptr);
        return;
    }
    // LOG(LOG_INFO) << "Successfully ran Lua script: SkippableTransitions.luac";
}

// Runs at the beginning of GAME:start(), which is called by main() inside main.lua.
__declspec(naked) void __stdcall GameStartHookASM()
{
    __asm
    {
        push ecx
        call LoadSkipTransitionsScript
        pop ecx
        mov ebx, ecx
        mov [ebp - 0x24], ebx
        jmp jmpGameStartHookReturnAddr
    }
}

bool PatchCustomLuaScripts()
{
    auto lua_state_pattern = hook::pattern("8B 85 E4 FE FF FF 85 C0 74 09 50");
    RETURN_IF_PATTERN_NOT_FOUND(lua_state_pattern);
    BYTE* lua_state_addr = lua_state_pattern.count(1).get(0).get<BYTE>(0) + 0x1A;
    LuaStatePtr = (void**)(*(DWORD*)lua_state_addr);
    lua_pcall = (lua_pcall_func)(*(DWORD*)(lua_state_addr + 0x1B) + (DWORD)lua_state_addr + 0x1F);
    lua_tolstring = (lua_tolstring_func)(*(DWORD*)(lua_state_addr + 0x35) + (DWORD)lua_state_addr + 0x3F);

    auto lua_loadbuffer_pattern = hook::pattern("8A 01 41 84 C0 75 F9 68");
    RETURN_IF_PATTERN_NOT_FOUND(lua_loadbuffer_pattern);
    BYTE* lua_loadbuffer_addr = lua_loadbuffer_pattern.count(1).get(0).get<BYTE>(0) + 0x18;
    luaL_loadbuffer = (luaL_loadbuffer_func)(*(DWORD*)(lua_loadbuffer_addr) + (DWORD)lua_loadbuffer_addr + 0x04);

    auto load_script_pattern = hook::pattern("8B D9 89 5D DC 8B 0D");
    RETURN_IF_PATTERN_NOT_FOUND(load_script_pattern);
    BYTE* load_script_inject_addr = load_script_pattern.count(1).get(0).get<BYTE>(0);
    jmpGameStartHookReturnAddr = load_script_inject_addr + 0x05;

    LOG(LOG_INFO) << "Patching custom Lua script hook...";
    WriteJmp(load_script_inject_addr, GameStartHookASM, 0x05);
    return true;
}

}  // namespace vlr
