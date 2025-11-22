#include <iostream>
#include <string>
#include <vector>

// TODO: Derive path from argv
constexpr char kTmpFilepath[] = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Zero Escape The Nonary Games\\ze2_data_en_us.bin";
constexpr char kTmpOutFilepath[] = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Zero Escape The Nonary Games\\ze2_data_en_us\\tmp\\tmp.bin";
constexpr char kTmp2OutFilepath[] = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Zero Escape The Nonary Games\\ze2_data_en_us\\tmp\\00000000.bin";

constexpr char kBinKeyStr[] = "ZeroEscapeTNG";

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

void DecryptWithKey(uint32_t* dst, uint8_t* src, uint32_t size, uint32_t key, uint32_t unk)
{
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

struct BinHeader
{
    uint32_t magic;  // "bin."
    uint32_t directory_table_offs;
    uint32_t file_table_offs;
    uint32_t data_offs;
    uint32_t unk_0x10;
    uint32_t total_meta_size;
    uint32_t unk_0x18;
    uint32_t reserved;
};

struct DirectoryTableHeader
{
    uint32_t start_offs;
    uint32_t count;
    uint32_t reserved[2];
};

struct DirectoryEntry
{
    uint32_t hash;
    uint32_t file_count;
    uint32_t file_start_index;
    uint32_t reserved;
};

struct FileTableHeader
{
    uint32_t start_offs;
    uint32_t count;
    uint32_t reserved[2];
};

struct FileEntry
{
    uint32_t data_offs;
    uint32_t unk_0x04;
    uint32_t hash;
    uint32_t bytesize;
    uint32_t unk_0x10;
    uint32_t file_index;
    uint16_t group_index;
    uint16_t unk_0x1A;
    uint32_t reserved;
};

bool ReadHeader(FILE* bin_file, int key, BinHeader* header_out)
{
    // FUN_00658820
    if (bin_file == nullptr || header_out == nullptr) return false;

    int readlen = fread(header_out, 1, 0x20, bin_file);
    if (readlen != 0x20)
    {
        std::cout << "Failed to read archive header" << std::endl;
        return false;
    }
    DecryptWithKey((uint32_t*)header_out, (uint8_t*)header_out, 0x20, key, /*unk=*/0);
    if (header_out->magic != 0x2E6E6962)  // "bin."
    {
        std::cout << "Invalid magic at offset 0x00" << std::endl;
        return false;
    }
    return true;
}

void OpenBinFile(const char* filename)
{
    FILE* bin_file;
    char head_buffer[0x100];
    errno_t err = fopen_s(&bin_file, filename, "rb");
    if (err != 0)
    {
        strerror_s(head_buffer, sizeof(head_buffer), err);
        std::cout << "Failed to open archive: " << head_buffer << std::endl;
        return;
    }

    // Read top-level header
    uint32_t bin_key = CalculateKey(kBinKeyStr);
    BinHeader header;
    if (!ReadHeader(bin_file, bin_key, &header))
    {
        fclose(bin_file);
        return;
    }

    // Read all table metadata
    std::vector<char> buffer;
    buffer.resize(header.total_meta_size);
    fseek(bin_file, 0, SEEK_SET);
    int readlen = fread(buffer.data(), 1, header.total_meta_size, bin_file);
    if (readlen != header.total_meta_size)
    {
        std::cout << "Failed to read BIN metadata" << std::endl;
        fclose(bin_file);
        return;
    }
    DecryptWithKey((uint32_t*)buffer.data(), (uint8_t*)buffer.data(), header.total_meta_size, bin_key, /*unk=*/0);

    // Write all metadata to a temporary file
    FILE* out_file;
    err = fopen_s(&out_file, kTmpOutFilepath, "wb+");
    if (err != 0)
    {
        strerror_s(head_buffer, sizeof(head_buffer), err);
        std::cout << "Failed to open for writing: " << kTmpOutFilepath << std::endl;
        return;
    }
    fwrite(buffer.data(), 1, header.total_meta_size, out_file);
    fclose(out_file);

    // Decrypt and write the first file
    const FileTableHeader* file_table_header = reinterpret_cast<FileTableHeader*>(buffer.data() + header.file_table_offs);
    const FileEntry* first_file_entry = reinterpret_cast<FileEntry*>(buffer.data() + header.file_table_offs + file_table_header->start_offs);

    std::vector<char> first_file;
    first_file.resize(first_file_entry->bytesize);

    fseek(bin_file, header.data_offs + first_file_entry->data_offs, SEEK_SET);
    readlen = fread(first_file.data(), 1, first_file_entry->bytesize, bin_file);
    fclose(bin_file);
    if (readlen != first_file_entry->bytesize)
    {
        std::cout << "Failed to read first file" << std::endl;
        return;
    }
    DecryptWithKey((uint32_t*)first_file.data(), (uint8_t*)first_file.data(), first_file_entry->bytesize, first_file_entry->hash, /*unk=*/0);

    err = fopen_s(&out_file, kTmp2OutFilepath, "wb+");
    if (err != 0)
    {
        strerror_s(head_buffer, sizeof(head_buffer), err);
        std::cout << "Failed to open for writing: " << kTmp2OutFilepath;
        return;
    }
    fwrite(first_file.data(), 1, first_file_entry->bytesize, out_file);
    fclose(out_file);

    std::cout << "Done." << std::endl;

    // std::cout << "/script.zip: " << std::hex << CalculateKey("/script.zip") << std::endl;
}

int main(int argc, char** argv)
{
    /*std::cout << CalculateKey("/scenes/ui/plusicon.zip") << std::endl;
    return 0;*/

    OpenBinFile(kTmpFilepath);
    std::cout << std::endl;
    return 0;
}
