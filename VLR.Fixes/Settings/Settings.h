#pragma once

#include <string>

struct IntSetting
{
    const char* section;
    const char* name;
    int default_value = 0;
    int value = 0;

    void Init(const char* settings_filepath);

    explicit operator bool() const {
        return value;
    }
};

struct Settings
{
    Settings();

    IntSetting DisableLogging{ .section = "Core", .name = "DisableLogging", .default_value = 0 };

    IntSetting LipAnimationFix{ .section = "Patches", .name = "LipAnimationFix", .default_value = 1 };
    IntSetting SkippableTransitions{ .section = "Patches", .name = "SkippableTransitions", .default_value = 1 };
    IntSetting CustomGameFiles{ .section = "Patches", .name = "CustomGameFiles", .default_value = 1 };
};
