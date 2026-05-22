#include <iostream>
#include <string>
#include <vector>

#include "BinArchive.h"
#include "Utils.h"

constexpr char kDefaultHashListPath[] = "ze2_en_us_hashlist.csv";

void PrintUsage()
{
    std::cout << "Usage: VLR.Extractor.exe path\\to\\bin [--out output\\path\\to\\extract\\files] [--hashlist path\\to\\hashlist.csv]" << std::endl;
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

    int i = 2;
    while (i < argc - 1)
    {
        const std::string arg = argv[i];
        if (arg == "--out")
        {
            output_path = std::string(argv[i + 1]);
            i += 2;
        }
        else if (arg == "--hashlist")
        {
            hashlist_path = std::string(argv[i + 1]);
            i += 2;
        }
        else
        {
            std::cout << "Unrecognized parameter: " << arg << std::endl << std::endl;
            PrintUsage();
            return 1;
        }
    }

    BinArchive bin_archive;
    if (!bin_archive.Open(bin_path, kDefaultHashListPath))
    {
        return 1;
    }
    if (!bin_archive.ExtractTo(output_path))
    {
        return 1;
    }

    std::cout << std::endl << "Done extracting " << bin_archive.GetFileCount() << " files." << std::endl;
    return 0;
}
