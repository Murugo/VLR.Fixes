#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class BinArchive
{
public:
    BinArchive() = default;
    ~BinArchive();

    bool Open(const std::string& bin_path, const std::string& hashlist_csv_path);
    bool ExtractTo(const std::string& output_path);

    int GetFileCount() { return files_.size(); }

private:
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

    struct FileTableHeader
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

    struct FileEntry
    {
        uint32_t data_offs;
        uint32_t unk_0x04;
        uint32_t hash;
        uint32_t bytesize;
        uint32_t unk_0x10;
        uint32_t file_index;
        uint16_t dir_index;
        uint16_t unk_0x1A;
        uint32_t reserved;
    };

    bool ReadHeader(int key, BinHeader* header_out);
    bool ReadDirectoryEntries(const char* data, uint32_t offs);
    bool ReadFileEntries(const char* data, uint32_t offs);
    std::string GetDirectoryPath(int index, const std::string& root_path);
    std::string GetFilePath(const FileEntry& entry, const std::string& root_path);
    bool ParseHashList(const std::string& hashlist_csv_path);
    void GuessExtensionPerDirectory();

    FILE* f_ = nullptr;
    std::unordered_map<uint32_t, std::string> hash_to_filepath_;
    std::unordered_map<uint32_t, std::string> dir_index_to_filepath_;
    std::unordered_map<uint32_t, std::string> dir_to_guessed_ext_;
    std::vector<DirectoryEntry> dirs_;
    std::vector<FileEntry> files_;
    uint32_t data_offset_ = 0;
};
