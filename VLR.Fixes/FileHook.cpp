#include "pch.h"

#include "FileHook.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <cctype>

#include "Logging.h"
#include "MemoryUtils.h"
#include "Utils\BloomFilter.h"
#include "..\External\Hooking.Patterns\Hooking.Patterns.h"

namespace vlr {

namespace {

constexpr char kWinfiles[] = "../winfiles/";
constexpr char kCustomFilesDir[] = "VLR.Fixes";

static DWORD CopyStringFuncAddr = 0;
static BYTE* jmpLoadGameFileReturnAddr1 = nullptr;
static BYTE* jmpLoadGameFileReturnAddr2 = nullptr;
static BYTE* jmpFileExistsReturnAddr = nullptr;
static BYTE* jmpRebuildFilterReturnAddr = nullptr;

static bool DebugPrintPath = false;
static std::unique_ptr<BloomFilter> path_bloom_filter;
static std::mutex mutex;

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

std::filesystem::path GetNormalizedPath(char* filename)
{
    // Strip "../winfiles/" prefix from file path
    const size_t winfiles_len = strlen(kWinfiles);
    if (!strncmp(filename, kWinfiles, winfiles_len))
    {
        filename += winfiles_len;
    }
    return std::filesystem::path(filename).make_preferred();
}

std::filesystem::path GetAbsoluteLocalPath(const std::filesystem::path& filename_path)
{
    return std::filesystem::current_path() / std::filesystem::path(kCustomFilesDir) / filename_path;
}

bool LoadGameFile(char* filename, PathString* path_string_out)
{
    std::scoped_lock lock(mutex);
    if (filename == nullptr) return false;

    const auto filename_path = GetNormalizedPath(filename);
    if (filename_path.empty() || filename_path.is_absolute())
    {
        return false;
    }
    const auto local_path = GetAbsoluteLocalPath(filename_path);
    if (DebugPrintPath)
    {
        OutputDebugStringA(local_path.string().c_str());
    }
    if (!path_bloom_filter || !path_bloom_filter->Exists(filename_path) || !std::filesystem::exists(local_path))
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

// Returns true if the file can be found in the custom game files directory.
bool CustomGameFileExists(char* filename)
{
    std::scoped_lock lock(mutex);

    if (filename == nullptr || !path_bloom_filter) return false;

    const auto filename_path = GetNormalizedPath(filename);
    if (filename_path.empty() || filename_path.is_absolute() || !path_bloom_filter->Exists(filename_path))
    {
        return false;
    }
    const auto local_path = GetAbsoluteLocalPath(filename_path);
    return std::filesystem::exists(local_path);
}

__declspec(naked) void __stdcall FileExistsASM()
{
    __asm
    {
        mov ebx, [ebp + 0x08]
        push ecx
        mov ebx, [ebx]
        push ebx
        call CustomGameFileExists
        add esp, 0x04
        pop ecx
        test al, al
        jz ExitASM
        pop ebx
        pop ebp
        ret 0x04

    ExitASM:
        mov ebx, [ebp + 0x08]
        push esi
        push edi
        jmp jmpFileExistsReturnAddr
    }
}

void RebuildPathBloomFilter()
{
    std::scoped_lock lock(mutex);

    std::filesystem::path custom_files_dir = std::filesystem::path(kCustomFilesDir);
    if (!std::filesystem::exists(custom_files_dir))
    {
        path_bloom_filter.reset(nullptr);
        return;
    }

    // LOG(LOG_INFO) << "Rebuilding custom game files index...";
    int file_count = 0;
    for (const auto& file_entry : std::filesystem::recursive_directory_iterator(custom_files_dir))
    {
        if (!file_entry.is_regular_file()) continue;
        const auto relative_path = std::filesystem::relative(file_entry.path(), custom_files_dir);
        file_count++;
    }
    // LOG(LOG_INFO) << "Found " << file_count << " custom game files.";
    if (file_count == 0)
    {
        path_bloom_filter.reset(nullptr);
        return;
    }

    path_bloom_filter.reset(new BloomFilter(file_count, 0.02));
    for (const auto& file_entry : std::filesystem::recursive_directory_iterator(custom_files_dir))
    {
        if (!file_entry.is_regular_file()) continue;
        const auto relative_path = std::filesystem::relative(file_entry.path(), custom_files_dir);
        path_bloom_filter->Insert(relative_path);
    }
}

// Rebuilds the custom game file filter inside the Lua call to GAME:start().
__declspec(naked) void __stdcall RebuildFilterASM()
{
    __asm
    {
        push eax
        call RebuildPathBloomFilter
        pop eax
        mov esi, eax
        add esp, 0x14
        jmp jmpRebuildFilterReturnAddr
    }
}

}  // namespace

bool PatchCustomGameFiles(const Settings& settings)
{
    auto load_game_file_pattern = hook::pattern("8B F9 FF 75 08 8D 4D E8");
    RETURN_IF_PATTERN_NOT_FOUND(load_game_file_pattern);
    BYTE* load_game_file_inject_addr = load_game_file_pattern.count(1).get(0).get<BYTE>(0) + 0x02;
    jmpLoadGameFileReturnAddr1 = load_game_file_inject_addr + 0x06;
    jmpLoadGameFileReturnAddr2 = load_game_file_inject_addr + 0x20;
    CopyStringFuncAddr = *(DWORD*)(load_game_file_inject_addr + 0x1C) + (DWORD)load_game_file_inject_addr + 0x20;

    auto file_exists_pattern = hook::pattern("8B 5D 08 56 57 8B F9 33 F6 39 77 40");
    RETURN_IF_PATTERN_NOT_FOUND(file_exists_pattern);
    BYTE* file_exists_inject_addr = file_exists_pattern.count(1).get(0).get<BYTE>(0);
    jmpFileExistsReturnAddr = file_exists_inject_addr + 0x05;

    auto rebuild_filter_pattern = hook::pattern("8B F0 83 C4 14 89 75 EC");
    RETURN_IF_PATTERN_NOT_FOUND(rebuild_filter_pattern);
    BYTE* rebuild_filter_inject_addr = rebuild_filter_pattern.count(1).get(0).get<BYTE>(0);
    jmpRebuildFilterReturnAddr = rebuild_filter_inject_addr + 0x05;

    DebugPrintPath = settings.DebugPrintGameFilePaths.value;

    LOG(LOG_INFO) << "Patching game file hook...";
    WriteJmp(load_game_file_inject_addr, LoadGameFileASM, 0x06);
    WriteJmp(file_exists_inject_addr, FileExistsASM, 0x05);
    WriteJmp(rebuild_filter_inject_addr, RebuildFilterASM, 0x05);
    RebuildPathBloomFilter();
    return true;
}

}  // namespace vlr
