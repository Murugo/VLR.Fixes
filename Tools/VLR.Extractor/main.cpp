#include <iostream>
#include <string>
#include <vector>

#include "BinArchive.h"
#include "Utils.h"

constexpr char kDefaultHashListPath[] = "ze2_en_us_hashlist.csv";

void PrintUsage()
{
    std::cout << "Usage: VLR.Extractor.exe path\\to\\bin [--overwrite] [--out output\\path\\to\\extract\\files] [--hashlist path\\to\\hashlist.csv]" << std::endl;
}

int main(int argc, char** argv)
{
    /*std::cout << CalculateKey("/scenes/ui/plusicon.zip") << std::endl;
    return 0;*/

    if (argc < 2)
    {
        PrintUsage();
        return 0;
    }
    const std::string bin_path = argv[1];
    std::string output_path = bin_path + "_out";
    std::string hashlist_path = kDefaultHashListPath;
    bool overwrite = false;

    int i = 2;
    while (i < argc)
    {
        const std::string arg = argv[i];
        if (arg == "--out" && i < argc - 1)
        {
            output_path = std::string(argv[i + 1]);
            i += 2;
        }
        else if (arg == "--hashlist" && i < argc - 1)
        {
            hashlist_path = std::string(argv[i + 1]);
            i += 2;
        }
        else if (arg == "--overwrite")
        {
            overwrite = true;
            i++;
        }
        else
        {
            std::cout << "Invalid parameter: " << arg << std::endl << std::endl;
            PrintUsage();
            return 1;
        }
    }

    BinArchive bin_archive;
    if (!bin_archive.Open(bin_path, kDefaultHashListPath))
    {
        return 1;
    }
    if (!bin_archive.ExtractTo(output_path, overwrite))
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
