#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "BinArchive.h"
#include "Utils.h"

void PrintUsage()
{
    std::cout << "Usage: VLR.Extractor.exe path\\to\\bin [--overwrite] [--out output\\path\\to\\extract\\files] [--hashlist path\\to\\hashlist.csv]" << std::endl;
}

bool GetInputYes()
{
    std::string response;
    std::cin >> response;
    std::transform(
        response.begin(), response.end(), response.begin(),
        [](unsigned char c) { return std::tolower(c); }
    );
    return response == "y" || response == "yes";
}

bool GetHashlistPath(const std::string& bin_path, std::string& hashlist_path)
{
    // Try to infer based on the BIN filename
    const std::string basename = std::filesystem::path(bin_path).stem().string();
    hashlist_path = basename + ".csv";
    if (std::filesystem::exists(hashlist_path))
    {
        return true;
    }

    // Otherwise, ask the user which list to use
    std::vector<std::string> candidates;
    for (const auto& file : std::filesystem::directory_iterator("."))
    {
        if (file.is_regular_file() && file.path().extension() == ".csv")
        {
            candidates.push_back(file.path().filename().string());
        }
    }
    if (!candidates.empty())
    {
        std::cout << "Select a hashlist for the input BIN file:" << std::endl << std::endl;
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            std::cout << i + 1 << ": " << candidates[i] << std::endl;
        }
        std::cout << candidates.size() + 1 << ": " << "(None)" << std::endl;
        std::cout << std::endl << "Enter #: ";

        size_t index;
        std::cin >> index;
        if (index < 1 || index > candidates.size() + 1)
        {
            return false;
        }
        else if (index == candidates.size() + 1)
        {
            std::cout << std::endl << "Extracted files will not have original filenames. Are you sure? (y/n): ";
            if (!GetInputYes())
            {
                return false;
            }
            hashlist_path = "";
        }
        else {
            hashlist_path = candidates[index - 1];
        }
    }
    else
    {
        std::cout << "No hashlist found. Extracted files will not have original filenames. Proceed? (y/n): ";
        if (!GetInputYes())
        {
            return false;
        }
        hashlist_path = "";
    }
    return true;
}

struct Options
{
    std::string bin_path;
    std::string output_path;
    std::string hashlist_path;
    bool overwrite = false;
    bool skip_unmatched = false;
};

bool ParseArgs(int argc, char** argv, Options& options)
{
    if (argc < 2)
    {
        PrintUsage();
        return 0;
    }
    options.bin_path = argv[1];
    options.output_path = options.bin_path + "_out";

    int i = 2;
    while (i < argc)
    {
        const std::string arg = argv[i];
        if (arg == "--out" && i < argc - 1)
        {
            options.output_path = std::string(argv[i + 1]);
            i += 2;
        }
        else if (arg == "--hashlist" && i < argc - 1)
        {
            options.hashlist_path = std::string(argv[i + 1]);
            i += 2;
        }
        else if (arg == "--overwrite")
        {
            options.overwrite = true;
            i++;
        }
        else if (arg == "--skip_unmatched")
        {
            options.skip_unmatched = true;
            i++;
        }
        else
        {
            std::cout << "Error: Invalid parameter: " << arg << std::endl << std::endl;
            PrintUsage();
            return 1;
        }
    }

    if (options.bin_path.empty())
    {
        std::cout << "Error: BIN path must be non-empty" << std::endl;
        return false;
    }
    if (options.output_path.empty())
    {
        std::cout << "Error: --output_path must be non-empty" << std::endl;
        return false;
    }
    if (options.hashlist_path.empty())
    {
        if (!GetHashlistPath(options.bin_path, options.hashlist_path))
        {
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv)
{
    Options options;
    if (!ParseArgs(argc, argv, options))
    {
        return 1;
    }

    BinArchive bin_archive;
    if (!bin_archive.Open(options.bin_path, options.hashlist_path) ||
        !bin_archive.ExtractTo(options.output_path, options.overwrite, options.skip_unmatched))
    {
        return 1;
    }
    if (const int skipped = bin_archive.GetFilesSkippedCount(); skipped > 0)
    {
        std::cout << std::endl << "Skipped " << skipped << " existing file" << (skipped > 1 ? "s" : "") << "." << std::endl;
    }
    if (const int extracted = bin_archive.GetFilesExtractedCount(); extracted > 0)
    {
        std::cout << std::endl << "Done extracting " << extracted << " file" << (extracted > 1 ? "s" : "") << "." << std::endl;
    }
    return 0;
}
