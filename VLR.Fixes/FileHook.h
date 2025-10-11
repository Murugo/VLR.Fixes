#pragma once

#include "Settings\Settings.h"

namespace vlr {

// Loads unpacked game files if they exist.
bool PatchCustomGameFiles(const Settings& settings);

}  // namespace vlr
