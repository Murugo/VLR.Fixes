#include "BinArchive.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Utils.h"

namespace {

constexpr char kBinKeyStr[] = "ZeroEscapeTNG";
constexpr char kDefaultFileExtension[] = ".bin";

}  // namespace

BinArchive::~BinArchive()
{
    if (f_)
    {
        fclose(f_);
    }
}

bool BinArchive::Open(const std::string& bin_path, const std::string& hashlist_csv_path)
{
    if (!ParseHashList(hashlist_csv_path))
    {
        return false;
    }
    const errno_t err = fopen_s(&f_, bin_path.c_str(), "rb");
    if (err != 0)
    {
        char err_buffer[0x100];
        strerror_s(err_buffer, sizeof(err_buffer), err);
        std::cerr << "Error: Failed to open archive: " << err_buffer << std::endl;
        return false;
    }

    // Readtop-level header
    const uint32_t bin_key = CalculateKey(kBinKeyStr);
    // uint32_t bin_key = 0xFABACEDA;  // 999
    BinHeader header;
    if (!ReadHeader(bin_key, &header))
    {
        return false;
    }
    data_offset_ = header.data_offs;

    // Read all table metadata
    std::vector<char> buffer;
    buffer.resize(header.total_meta_size);
    fseek(f_, 0, SEEK_SET);
    int readlen = fread(buffer.data(), 1, header.total_meta_size, f_);
    if (readlen != header.total_meta_size)
    {
        std::cerr << "Error: Failed to read archive metadata" << std::endl;
        return false;
    }
    DecryptWithKey((uint32_t*)buffer.data(), (uint8_t*)buffer.data(), header.total_meta_size, bin_key, /*unk=*/0);

    return ReadDirectoryEntries(buffer.data(), header.directory_table_offs) && ReadFileEntries(buffer.data(), header.file_table_offs);
}

bool BinArchive::ReadHeader(int key, BinHeader* header_out)
{
    // FUN_00658820
    int readlen = fread(header_out, 1, 0x20, f_);
    if (readlen != 0x20)
    {
        std::cout << "Failed to read archive header" << std::endl;
        return false;
    }
    DecryptWithKey((uint32_t*)header_out, (uint8_t*)header_out, 0x20, key, /*unk=*/0);
    if (header_out->magic != 0x2E6E6962)  // "bin."
    {
        std::cout << "Invalid magic at offset 0x00: " << std::hex << header_out->magic << std::endl;
        return false;
    }
    return true;
}

bool BinArchive::ReadDirectoryEntries(const char* data, uint32_t offs)
{
    if (data == nullptr)
    {
        return false;
    }
    const auto header = reinterpret_cast<const DirectoryTableHeader*>(data + offs);
    dirs_.reserve(header->count);
    for (uint32_t i = 0; i < header->count; ++i)
    {
        const auto entry = reinterpret_cast<const DirectoryEntry*>(data + offs + header->start_offs + i * 0x10);
        dirs_.push_back(*entry);
    }
    return true;
}

bool BinArchive::ReadFileEntries(const char* data, uint32_t offs)
{
    if (data == nullptr)
    {
        return false;
    }
    const auto header = reinterpret_cast<const FileTableHeader*>(data + offs);
    files_.reserve(header->count);
    for (uint32_t i = 0; i < header->count; ++i)
    {
        const auto entry = reinterpret_cast<const FileEntry*>(data + offs + header->start_offs + i * 0x20);
        files_.push_back(*entry);
    }
    GuessExtensionPerDirectory();
    return true;
}

bool BinArchive::ExtractTo(const std::string& output_path, bool overwrite)
{
    // Create all directories
    std::vector<std::string> dir_paths;
    dir_paths.reserve(dirs_.size());
    for (size_t i = 0; i < dirs_.size(); ++i)
    {
        const std::string dir_path = GetDirectoryPath(i, output_path);
        // std::cout << "Creating directory: " << dir_path << std::endl;
        std::filesystem::create_directories(dir_path);
        dir_paths.push_back(dir_path);
    }

    // Extract all files
    for (const FileEntry& entry : files_)
    {
        const std::string file_path = GetFilePath(entry, output_path);
        if (!overwrite && std::filesystem::exists(file_path))
        {
            files_skipped_++;
            continue;
        }
        files_extracted_++;
        std::cout << "Extracting: " << file_path << std::endl;

        std::vector<char> buffer;
        buffer.resize(entry.bytesize);

        const uint32_t offs = data_offset_ + entry.data_offs;
        _fseeki64(f_, offs, SEEK_SET);
        const int readlen = fread(buffer.data(), 1, entry.bytesize, f_);
        if (readlen != entry.bytesize)
        {
            std::cerr << "Error: Failed to read file at offset " << std::hex << offs << std::endl;
            return false;
        }
        DecryptWithKey((uint32_t*)buffer.data(), (uint8_t*)buffer.data(), entry.bytesize, entry.hash, /*unk=*/0);

        FILE* out_file;
        const errno_t err = fopen_s(&out_file, file_path.c_str(), "wb+");
        if (err != 0)
        {
            char err_buffer[0x100];
            strerror_s(err_buffer, sizeof(err_buffer), err);
            std::cerr << "Error: Failed to open for writing: " << file_path;
            return false;
        }
        fwrite(buffer.data(), 1, entry.bytesize, out_file);
        fclose(out_file);
    }
    return true;
}

std::string BinArchive::GetDirectoryPath(int index, const std::string& root_path)
{
    std::string dir;
    auto it = dir_index_to_filepath_.find(index);
    if (it != dir_index_to_filepath_.end())
    {
        dir = it->second;
    }
    else
    {
        std::stringstream dir_ss;
        dir_ss << "_Unmatched_Dir." << std::setfill('0') << std::setw(2) << index;
        dir = dir_ss.str();
    }
    return (std::filesystem::path(root_path) / std::filesystem::path(dir)).string();
}

std::string BinArchive::GetFilePath(const FileEntry& entry, const std::string& root_path)
{
    auto it = hash_to_filepath_.find(entry.hash);
    if (it != hash_to_filepath_.end())
    {
        return (std::filesystem::path(root_path) / std::filesystem::path(it->second)).string();
    }
    else
    {
        std::stringstream file_ss;
        file_ss << "_Unmatched_File." << std::setfill('0') << std::setw(4) << entry.file_index;
        
        auto it = dir_to_guessed_ext_.find(entry.dir_index);
        file_ss << (it != dir_to_guessed_ext_.end() ? it->second : kDefaultFileExtension);

        const std::string dir_path = GetDirectoryPath(entry.dir_index, root_path);
        return (std::filesystem::path(dir_path) / std::filesystem::path(file_ss.str())).string();
    }
}

bool BinArchive::ParseHashList(const std::string& csv_path)
{
    std::ifstream file(csv_path);
    if (!file.is_open())
    {
        std::cerr << "Error: Failed to open hashlist: " << csv_path << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream line_ss(line);
        std::string column;
        std::vector<std::string> columns;
        while (std::getline(line_ss, column, ','))
        {
            columns.push_back(column);
        }
        if (columns.size() < 4 || columns[2].empty() || columns[3].empty())
        {
            continue;
        }
        const uint32_t hash = std::stoi(columns[2].c_str());
        const uint32_t dir_index = std::stoi(columns[1].c_str());
        std::string path = columns[3].substr(columns[3].starts_with('/') ? 1 : 0);
        std::replace(path.begin(), path.end(), '/', '\\');
        hash_to_filepath_[hash] = path;
        if (dir_index_to_filepath_.find(dir_index) == dir_index_to_filepath_.end())
        {
            dir_index_to_filepath_[dir_index] = std::filesystem::path(path).parent_path().string();
        }
    }
    return true;
}

void BinArchive::GuessExtensionPerDirectory()
{
    for (const FileEntry& file : files_)
    {
        const DirectoryEntry& dir = dirs_[file.dir_index];
        if (dir.file_count < 4)
        {
            continue;
        }

        auto filepath_it = hash_to_filepath_.find(file.hash);
        if (filepath_it == hash_to_filepath_.end())
        {
            continue;
        }
        const std::string ext = std::filesystem::path(filepath_it->second).extension().string();

        auto guessed_ext_it = dir_to_guessed_ext_.find(file.dir_index);
        if (guessed_ext_it == dir_to_guessed_ext_.end())
        {
            dir_to_guessed_ext_[file.dir_index] = ext;
            continue;
        }
        if (!guessed_ext_it->second.empty() && guessed_ext_it->second != ext)
        {
            // Directory has multiple file extensions, so we set a default instead.
            dir_to_guessed_ext_[file.dir_index] = kDefaultFileExtension;
        }
    }
}
