#include "pch.h"

#include "LipAnim.h"

#include "Logging.h"
#include "MemoryUtils.h"
#include "..\External\Hooking.Patterns\Hooking.Patterns.h"

namespace vlr {

namespace {

using VlrInsertValueAtFunc = void(*)(void*, float*, DWORD);
static VlrInsertValueAtFunc VlrInsertValueAt;
static BYTE* jmpUpdateStartKeyframesReturnAddr = nullptr;
static BYTE* jmpUpdateEndKeyframesReturnAddr = nullptr;
static BYTE* jmpClampPhonemeStartTimeReturnAddr = nullptr;
static BYTE* jmpSkipMorphResetReturnAddr1 = nullptr;
static BYTE* jmpSkipMorphResetReturnAddr2 = nullptr;

struct KeyframeList
{
    float* values;
    DWORD reserved_size;
    DWORD size;
};

void InsertValueAt(KeyframeList* keyframe_list, float* value, DWORD index)
{
    __asm
    {
        mov ecx, dword ptr ds : [keyframe_list]
        push index
        push value
        mov eax, dword ptr ds : [VlrInsertValueAt]
        call eax
    }
}

// Fades in the default morph target at the start of the animation.
void UpdateLipStartKeyframes(DWORD* morph_anim)
{
    if (morph_anim == nullptr) return;

    // Disable at time 0.
    KeyframeList* value_list = (KeyframeList*)(morph_anim + 5);
    value_list->values[0] = 0.0f;

    // Insert a keyframe to enable at time 0.07.
    float time = 0.07f;
    KeyframeList* time_list = (KeyframeList*)morph_anim;
    InsertValueAt(time_list, &time, time_list->size);

    float value = 1.0f;
    InsertValueAt(value_list, &value, value_list->size);

    float unk = 0.0f;
    KeyframeList* unk_list = (KeyframeList*)(morph_anim + 20);
    InsertValueAt(unk_list, &unk, unk_list->size);
}

__declspec(naked) void __stdcall UpdateLipStartKeyframesASM()
{
    __asm
    {
        push dword ptr ds : [ebp - 0x78]
        call UpdateLipStartKeyframes

        xor ecx, ecx
        xor edi, edi
        mov dword ptr ds : [ebp - 0x64], ecx
        jmp jmpUpdateStartKeyframesReturnAddr
    }
}

// Fades out the default morph target at the end of the animation to unset the default mouth shape.
void UpdateLipEndKeyframes(DWORD* morph_anim, float* end_time)
{
    if (morph_anim == nullptr || end_time == nullptr) return;
    if (*end_time - 0.1f < 1e-5)
    {
        // Animation contains no phonemes, hence no fade-out is needed.
        // Update values for the last 2 keyframes directly.
        KeyframeList* value_list = (KeyframeList*)(morph_anim + 5);
        value_list->values[value_list->size - 1] = 0.0f;
        value_list->values[value_list->size - 2] = 0.0f;
    }
    else
    {
        // Adjust the time of the last keyframe to start the fade-out.
        KeyframeList* time_list = (KeyframeList*)morph_anim;
        time_list->values[time_list->size - 1] = *end_time - 0.1f;

        // Insert a new keyframe to fade out the default morph target
        // exactly when the animation ends.
        InsertValueAt(time_list, end_time, time_list->size);

        float value = 0.0f;
        KeyframeList* value_list = (KeyframeList*)(morph_anim + 5);
        InsertValueAt(value_list, &value, value_list->size);

        float unk = 0.0f;
        KeyframeList* unk_list = (KeyframeList*)(morph_anim + 20);
        InsertValueAt(unk_list, &unk, unk_list->size);
    }
}

__declspec(naked) void __stdcall UpdateLipEndKeyframesASM()
{
    __asm
    {
        lea eax, dword ptr ds : [ebp - 0x90]
        push eax
        push dword ptr ds : [ebp - 0x78]
        call UpdateLipEndKeyframes
        
        mov ecx, dword ptr ds : [ebp - 0xB4]
        jmp jmpUpdateEndKeyframesReturnAddr
    }
}

constexpr float kPhonemeStartTime = 0.001f;

// Clamps start time used for phonemes to prevent negative time values.
__declspec(naked) void __stdcall ClampPhonemeStartTimeASM()
{
    __asm
    {
        movss [ebp - 0x04], xmm0
        test dword ptr ds : [ebp - 0x04], 0x80000000
        jz ExitASM

        push eax
        mov eax, dword ptr ds : [kPhonemeStartTime]
        mov [ebp - 0x04], eax
        pop eax

    ExitASM:
        jmp jmpClampPhonemeStartTimeReturnAddr
    }
}

// Skips resetting morph target vertices of an active animation after
// the animation has already been initialized. This fix should only
// apply to animations where the character has both eyes closed.
//
// Some characters (e.g. Quark, Clover) are susceptible to a bug where
// if they have their eyes closed, and the blink animation plays while
// they are talking, their mouth shape will become corrupted. This is a
// special case where the blink itself inadvertently triggers all face
// vertices to reset. This issue is much more apparent with the lip
// animation fix enabled, hence the two fixes are bundled together.
__declspec(naked) void __stdcall SkipMorphResetASM()
{
    __asm
    {
        cmp esi, 0x7
        jne ResetOk

        mov esi, dword ptr ds : [esp + 0x14]
        mov ecx, dword ptr ds : [esp + 0x20]
        push eax
        mov eax, dword ptr ds : [esi + 0x2C]
        mov eax, dword ptr ds : [eax + ecx * 0x04]
        test eax, eax
        pop eax
        jnz SkipReset

        push eax
        mov eax, dword ptr ds : [esi + 0x18]
        mov eax, dword ptr ds : [eax + ecx * 0x04]
        cmp eax, 0x3F800000
        pop eax
        jnz SkipReset

    ResetOk:
        mov eax, [ebx]
        xor ecx, ecx
        mov dword ptr ds : [esp + 0x24], ecx
        jmp jmpSkipMorphResetReturnAddr1

    SkipReset:
        jmp jmpSkipMorphResetReturnAddr2
    }
}

}  // namespace

bool PatchLipAnimationFix()
{
    auto start_kf_pattern = hook::pattern("33 C9 33 FF 89 4D 9C");
    if (start_kf_pattern.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for start keyframe insertion";
        return false;
    }
    auto end_kf_pattern = hook::pattern("8B 8D 4C FF FF FF 83 EC 08");
    if (end_kf_pattern.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for end keyframe insertion";
        return false;
    }
    auto clamp_phoneme_pattern = hook::pattern("F3 0F 11 45 FC FF 77 08");
    if (clamp_phoneme_pattern.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for phoneme start time fix";
        return false;
    }
    auto skip_morph_reset_pattern = hook::pattern("8B 03 33 C9 89 4C 24 24");
    if (skip_morph_reset_pattern.size() != 1)
    {
        LOG(LOG_ERROR) << __FUNCTION__ << ": Failed to find memory address for skip morph reset fix";
        return false;
    }
    BYTE* start_kf_inject_addr = start_kf_pattern.count(1).get(0).get<BYTE>(0);
    BYTE* end_kf_inject_addr = end_kf_pattern.count(1).get(0).get<BYTE>(0);
    BYTE* clamp_phoneme_inject_addr = clamp_phoneme_pattern.count(1).get(0).get<BYTE>(0);
    BYTE* skip_morph_reset_addr = skip_morph_reset_pattern.count(1).get(0).get<BYTE>(0);
    VlrInsertValueAt = (VlrInsertValueAtFunc)(*(DWORD*)(end_kf_inject_addr - 0x19) + end_kf_inject_addr - 0x15);
    jmpUpdateStartKeyframesReturnAddr = start_kf_inject_addr + 7;
    jmpUpdateEndKeyframesReturnAddr = end_kf_inject_addr + 6;
    jmpClampPhonemeStartTimeReturnAddr = clamp_phoneme_inject_addr + 5;
    jmpSkipMorphResetReturnAddr1 = skip_morph_reset_addr + 8;
    jmpSkipMorphResetReturnAddr2 = skip_morph_reset_addr + 0xDB;

    LOG(LOG_INFO) << "Patching lip animation fix...";
    WriteJmp(start_kf_inject_addr, UpdateLipStartKeyframesASM, 7);
    WriteJmp(end_kf_inject_addr, UpdateLipEndKeyframesASM, 6);
    WriteJmp(clamp_phoneme_inject_addr, ClampPhonemeStartTimeASM, 5);
    WriteJmp(skip_morph_reset_addr, SkipMorphResetASM, 8);
    return true;
}

}  // namespace vlr
