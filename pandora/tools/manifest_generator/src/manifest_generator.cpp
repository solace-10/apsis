#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <clipp.h>
#include <nlohmann/json.hpp>
#include <xxhash.h>

namespace fs = std::filesystem;

std::string GenerateHash(const fs::path& file)
{
    std::ifstream ifs(file, std::ios::in | std::ios::binary | std::ios::ate);
    if (ifs.good())
    {
        std::ifstream::pos_type fileSize = ifs.tellg();
        std::vector<char> bytes;
        bytes.reserve(static_cast<size_t>(fileSize));
        ifs.read(bytes.data(), fileSize);
        ifs.close();

        std::stringstream ss;
        ss << XXH3_64bits(bytes.data(), fileSize);
        return ss.str();
    }
    
    return std::string("");
}

bool BuildManifest(const fs::path& directory)
{
    using namespace nlohmann;
    json manifest = json::array();
    try 
    {
        if (fs::exists(directory) && fs::is_directory(directory)) 
        {
            const size_t prefixLength = directory.string().length();
            for (const auto& entry : fs::recursive_directory_iterator(directory)) 
            {
                if (fs::is_regular_file(entry.path()) && entry.path().filename() != "manifest.json")
                {
                    json manifestEntry;
                    const std::string relativePath(entry.path().string().substr(prefixLength));
                    manifestEntry["path"] = relativePath;
                    manifestEntry["size"] = fs::file_size(entry.path());
                    manifestEntry["hash"] = GenerateHash(entry.path());
                    manifest.push_back(manifestEntry);
                }
            }

            fs::path manifestFs(directory / "manifest.json");
            std::ofstream fs(manifestFs, std::ios::out);
            if (fs.good())
            {
                fs << manifest.dump(4);
                fs.close();
                std::cout << "Wrote manifest to " << manifestFs << std::endl;
                return true;
            }
            else
            {
                return false;
            }
        } 
        else 
        {
            std::cerr << "Directory does not exist or is not a directory." << std::endl;
            return false;
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "General exception: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char* argv[])
{
    std::string directory = "";

    clipp::group cli{
        clipp::value("manifest directory", directory)
    };

    if (!clipp::parse(argc, argv, cli)) 
    {
        std::cout << clipp::make_man_page(cli, argv[0]);
        return -1;
    }
    else
    {
        const bool success = BuildManifest(directory);
        return success ? 0 : -1;
    }
}