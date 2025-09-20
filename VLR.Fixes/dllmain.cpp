#include "pch.h"

#include <filesystem>

#include "FileHook.h"
#include "LipAnim.h"
#include "Logging.h"
#include "version.h"

// Returns whether the current process is named "ze2.exe" (to avoid patching "ze1.exe" by mistake).
bool ValidateExe()
{
    char proc_path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, proc_path, MAX_PATH)) return false;
    return std::filesystem::path(proc_path).filename().string() == "ze2.exe";
}

bool ApplyPatches()
{
    bool success = true;
    if (!vlr::PatchCustomGameFiles())
    {
        LOG(LOG_ERROR) << "Failed to apply game file hooks!";
    }
    if (!vlr::PatchLipAnimationFix())
    {
        LOG(LOG_ERROR) << "Failed to apply lip animation fix!";
        success = false;
    }
    return success;
}

void Init()
{
    if (!ValidateExe()) return;

    LOG(LOG_INFO) << "Virtue's Last Reward Fixes " << VER_FILE_VERSION_STR;

    if (ApplyPatches())
    {
        LOG(LOG_INFO) << "Applied all patches successfully.";
    }
    else
    {
        LOG(LOG_INFO) << "One or more patches failed to apply.";
    }
}

extern "C"
{
    __declspec(dllexport) void InitializeASI()
    {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;
        Init();
    }
}
