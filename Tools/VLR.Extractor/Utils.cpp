#include "Utils.h"

#include <cstdint>
#include <string>

uint32_t CalculateKey(const char* key)
{
    // FUN_006586B0
    const size_t len = strlen(key);
    uint32_t iVar5 = 0;
    uint32_t iVar6 = 0;
    uint32_t uVar7 = 0;
    size_t i = 0;
    if (len > 1)
    {
        while (i < len - 1)
        {
            const uint8_t a0 = (uint8_t)key[i];
            const uint8_t a1 = (uint8_t)key[i + 1];
            i += 2;
            iVar5 += a0;
            iVar6 += a1;
            uVar7 = ((a0 & 0xDF) + uVar7 * 0x83) * 0x83 + (a1 & 0xDF);
        }
    }
    uint32_t iVar4 = 0;
    if (i < len)
    {
        iVar4 = (uint32_t)key[i];
        uVar7 = uVar7 * 0x83 + (iVar4 & 0xDF);
    }
    return iVar6 + iVar5 + iVar4 & 0x0F | (uVar7 & 0x7FFFFFF) << 4;
}

void DecryptWithKey(uint32_t* dst, uint8_t* src, uint32_t size, uint32_t key, uint32_t unk) {
    // FUN_00653780
    uint32_t uVar3 = unk;
    uint32_t offset = reinterpret_cast<uint32_t>(src) - reinterpret_cast<uint32_t>(dst);
    uint32_t uVar4 = size;
    if ((unk & 3) == 0)
    {
        uVar4 = size >> 2;
        uint32_t* dst_ptr = dst;
        if (uVar4 != 0)
        {
            uint32_t uVar5 = (unk + 1) * 0x100;
            uint32_t uVar7 = (unk + 2) * 0x10000;
            uint32_t uVar10 = (unk + 3) * 0x1000000;
            while (uVar4 != 0)
            {
                *dst_ptr = (uVar7 & 0xFF0000 | uVar5 & 0xFF00 | uVar3 & 0xFF | uVar10) ^ *(uint32_t*)(reinterpret_cast<uint32_t>(dst_ptr) + offset) ^ key;
                uVar3 += 0x04;
                uVar5 += 0x400;
                uVar7 += 0x40000;
                uVar10 += 0x4000000;
                uVar4--;
                dst_ptr++;
            }
        }
        uVar4 = size & 3;
        dst = (uint32_t*)(reinterpret_cast<uint32_t>(dst) + size - uVar4);
        src += size - uVar4;
    }
    offset = reinterpret_cast<uint32_t>(dst) - reinterpret_cast<uint32_t>(src);
    uint8_t* src_ptr = src;
    while (uVar4 != 0)
    {
        src_ptr[offset] = *(uint8_t*)(reinterpret_cast<uint32_t>(&key) + (uVar3 & 0x03)) ^ (uVar3 & 0xFF) ^ *src_ptr;
        uVar3++;
        uVar4--;
        src_ptr++;
    }
}
