#include "..\pch.h"

#include "Settings.h"

#include <filesystem>

namespace {

constexpr char kSettingsFile[] = "VLR.Fixes.ini";

}  // namespace

void IntSetting::Init(const char* settings_filepath)
{
    value = GetPrivateProfileIntA(section, name, default_value, settings_filepath);
}

Settings::Settings()
{
    const std::string settings_path = std::filesystem::absolute(kSettingsFile).string();

    DisableLogging.Init(settings_path.c_str());

    LipAnimationFix.Init(settings_path.c_str());
    SkippableTransitions.Init(settings_path.c_str());
    CustomGameFiles.Init(settings_path.c_str());
}
