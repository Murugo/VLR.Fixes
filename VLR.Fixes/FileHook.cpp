#include "pch.h"

#include "FileHook.h"

#include <filesystem>

#include "Logging.h"
#include "MemoryUtils.h"
#include "..\External\Hooking.Patterns\Hooking.Patterns.h"

namespace vlr {

namespace {

constexpr char kWinfiles[] = "../winfiles/";
constexpr char kCustomFilesDir[] = "VLR.Fixes";

static DWORD CopyStringFuncAddr = 0;
static BYTE* jmpLoadGameFileReturnAddr1 = nullptr;
static BYTE* jmpLoadGameFileReturnAddr2 = nullptr;

struct PathString {
    const char* str = nullptr;
    DWORD reserved_size = 0;
    DWORD size = 0;
};

void CopyString(PathString* dst, PathString* src)
{
    __asm
    {
        mov ecx, dword ptr ds : [dst]
        push src
        mov eax, dword ptr ds : [CopyStringFuncAddr]
        call eax
        add esp, 0x04
    }
}

bool LoadGameFile(char* filename, PathString* path_string_out)
{
    if (filename == nullptr) return false;

    // Strip "../winfiles/" prefix from file path
    const size_t winfiles_len = strlen(kWinfiles);
    if (!strncmp(filename, kWinfiles, winfiles_len))
    {
        filename += winfiles_len;
    }
    const auto filename_path = std::filesystem::path(filename).make_preferred();
    if (filename_path.is_absolute())
    {
        return false;
    }
    const auto local_path =
        std::filesystem::current_path() /
        std::filesystem::path(kCustomFilesDir) /
        filename_path;

    OutputDebugStringA(local_path.string().c_str());

    if (!std::filesystem::exists(local_path))
    {
        return false;
    }

    const std::string local_path_str = local_path.string();
    PathString path_obj = {
        .str = local_path_str.c_str(),
        .reserved_size = local_path_str.size() + 1,
        .size = local_path_str.size() + 1
    };
    path_string_out->str = nullptr;
    path_string_out->reserved_size = 0;
    path_string_out->size = 0;
    CopyString(path_string_out, &path_obj);
    return true;
}

__declspec(naked) void __stdcall LoadGameFileASM()
{
    __asm
    {
        push edi
        mov eax, dword ptr ds : [ebp + 0x08]
        mov ecx, dword ptr ds : [eax + 0x08]
        test ecx, ecx
        jz ExitASM

        lea ecx, dword ptr ds : [ebp - 0x18]
        push ecx
        push dword ptr ds : [eax]
        call LoadGameFile
        add esp, 0x08
        test al, al
        jz ExitASM
        pop edi
        jmp jmpLoadGameFileReturnAddr2

    ExitASM:
        pop edi
        push dword ptr ds : [ebp + 0x08]
        lea ecx, dword ptr ds : [ebp - 0x18]
        jmp jmpLoadGameFileReturnAddr1
    }
}

}  // namespace

bool PatchCustomGameFiles()
{
    auto load_game_file_pattern = hook::pattern("8B F9 FF 75 08 8D 4D E8");
    if (load_game_file_pattern.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for loading game files";
        return false;
    }
    BYTE* load_game_file_inject_addr = load_game_file_pattern.count(1).get(0).get<BYTE>(0) + 0x02;
    jmpLoadGameFileReturnAddr1 = load_game_file_inject_addr + 6;
    jmpLoadGameFileReturnAddr2 = load_game_file_inject_addr + 0x20;
    CopyStringFuncAddr = *(DWORD*)(load_game_file_inject_addr + 0x1C) + (DWORD)load_game_file_inject_addr + 0x20;

    LOG(LOG_INFO) << "Patching game file hook...";
    WriteJmp(load_game_file_inject_addr, LoadGameFileASM, 6);
    return true;
}

}  // namespace vlr
