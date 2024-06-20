#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

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

void BuildManifest(const fs::path& directory)
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
            }
        } 
        else 
        {
            std::cerr << "Directory does not exist or is not a directory." << std::endl;
            exit(-1);
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        exit(-1);
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "General exception: " << e.what() << std::endl;
        exit(-1);
    }
}

int main(int argc, const char** argv)
{   
    BuildManifest("/home/solace10/dev/TheBrightestStar/game/bin/data/core");

    return 0;
}