#include "pch.h"

#include <filesystem>

#include "FileHook.h"
#include "LipAnim.h"
#include "Logging.h"
#include "LuaHook.h"
#include "resource.h"
#include "Settings\Settings.h"

// Returns whether the current process is named "ze2.exe" (to avoid patching "ze1.exe" by mistake).
bool ValidateExe()
{
    char proc_path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, proc_path, MAX_PATH)) return false;
    return std::filesystem::path(proc_path).filename().string() == "ze2.exe";
}

bool ApplyPatches(const Settings& settings)
{
    bool success = true;
    if (settings.CustomGameFiles)
    {
        if (!vlr::PatchCustomGameFiles(settings))
        {
            LOG(LOG_ERROR) << "Failed to apply patch CustomGameFiles!";
        }
    }
    if (settings.SkippableTransitions)
    {
        if (!vlr::PatchCustomLuaScripts())
        {
            LOG(LOG_ERROR) << "Failed to apply patch SkippableTransitions!";
        }
    }
    if (settings.LipAnimationFix)
    {
        if (!vlr::PatchLipAnimationFix())
        {
            LOG(LOG_ERROR) << "Failed to apply patch LipAnimationFix!";
            success = false;
        }
    }
    return success;
}

void Init()
{
    if (!ValidateExe()) return;

    Settings settings;
    if (settings.DisableLogging)
    {
        FileLogger::Get()->SetEnabled(false);
    }

    LOG(LOG_INFO) << "Virtue's Last Reward Fixes " << VER_FILE_VERSION_STR;

    if (ApplyPatches(settings))
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
