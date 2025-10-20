#include "pch.h"

#include "WindowState.h"

#include "Logging.h"
#include "MemoryUtils.h"
#include "..\External\Hooking.Patterns\Hooking.Patterns.h"

namespace vlr {

DWORD* WindowStatePtr = 0;

__declspec(naked) void __stdcall DisablePauseInBackgroundASM()
{
    __asm
    {
        push eax
        mov eax, dword ptr ds : [WindowStatePtr]
        cmp byte ptr ds : [eax], 00  // Not fullscreen?
        jz ExitASM
        add esp, 0x04
        pop esi
        mov al, 0x01
        ret

    ExitASM:
        pop eax
        cmp eax, dword ptr ds : [esi + 0xD8]
        pop esi
        setz al
        ret
    }
}

bool PatchDisablePauseInBackground()
{
    auto window_state_pattern = hook::pattern("85 C9 75 0B A1 98 E1 94 00");
    RETURN_IF_PATTERN_NOT_FOUND(window_state_pattern);

    auto vtable_pattern = hook::pattern("57 C7 85 B0 FE FF FF");
    RETURN_IF_PATTERN_NOT_FOUND(vtable_pattern);

    WindowStatePtr = (DWORD*)*window_state_pattern.count(1).get(0).get<DWORD>(-0x04);
    BYTE* hook_addr = vtable_pattern.count(1).get(0).get<BYTE>(0);
    BYTE* inject_addr = (BYTE*)*(DWORD*)(*(DWORD*)(hook_addr + 0x26) + 0x38) + 0x09;

    LOG(LOG_INFO) << "Patching no background pausing in windowed mode...";
    WriteJmp(inject_addr, DisablePauseInBackgroundASM, 0x06);
    return true;
}

}  // namespace vlr
