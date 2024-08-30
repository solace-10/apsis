#include <filesystem>
#include <fstream>

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <xxhash.h>

#include <core/log.hpp>

#include "manifest.hpp"

namespace WingsOfSteel::Forge
{

namespace fs = std::filesystem;

Manifest::Manifest()
{
    BuildFromFilesystem();
    OverrideFromPreviousManifest();
}

Manifest::~Manifest()
{

}

const std::vector<ManifestEntry*> Manifest::GetEntries() const
{
    std::vector<ManifestEntry*> entries;
    for (auto& pair : m_Entries)
    {
        entries.push_back(pair.second.get());
    }
    return entries;
}

void Manifest::Save()
{
    using namespace nlohmann;
    json manifest = json::array();
    try 
    {
        for (const auto& pair : m_Entries) 
        {
            ManifestEntry* pEntry = pair.second.get();

            json manifestEntry;
            manifestEntry["path"] = pEntry->GetPath();
            manifestEntry["size"] = pEntry->GetSize();
            manifestEntry["hash"] = pEntry->GetHash();
            if (pEntry->GetMode() != ManifestEntryMode::Background)
            {
                manifestEntry["mode"] = magic_enum::enum_name(pEntry->GetMode());
            }
            manifest.push_back(manifestEntry);
        }

        fs::path manifestFs("../../game/bin/data/core/manifest.json");
        std::ofstream fs(manifestFs, std::ios::out);
        if (fs.good())
        {
            fs << manifest.dump(4);
            fs.close();
            Pandora::Log::Info() << "Manifest file written out.";
        }
        else
        {
            Pandora::Log::Error() << "Failed to write out manifest file.";
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        Pandora::Log::Error() << "Filesystem error: " << e.what();
    } 
    catch (const std::exception& e) 
    {
        Pandora::Log::Error() << "General exception: " << e.what();
    }
}

void Manifest::BuildFromFilesystem()
{
    try 
    {
        std::filesystem::path directory = "../../game/bin/data/core";
        if (fs::exists(directory) && fs::is_directory(directory)) 
        {
            const size_t prefixLength = directory.string().length();
            for (const auto& entry : fs::recursive_directory_iterator(directory)) 
            {
                if (fs::is_regular_file(entry.path()) && entry.path().filename() != "manifest.json")
                {
                    std::string relativePath(entry.path().string().substr(prefixLength));
                    std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

                    ManifestEntryUniquePtr pManifestEntry = std::make_unique<ManifestEntry>(
                        relativePath,
                        fs::file_size(entry.path()),
                        GenerateHash(entry.path()),
                        ManifestEntryMode::Background
                    );

                    m_Entries[relativePath] = std::move(pManifestEntry);
                }
            }
        } 
        else 
        {
            Pandora::Log::Error() << "Directory does not exist or is not a directory.";
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        Pandora::Log::Error() << "Filesystem error: " << e.what();
    } 
    catch (const std::exception& e) 
    {
        Pandora::Log::Error() << "General exception: " << e.what();
    }
}
    
void Manifest::OverrideFromPreviousManifest()
{
    using namespace nlohmann;

    std::ifstream manifestFile("../../game/bin/data/core/manifest.json");
    if (!manifestFile)
    {
        Pandora::Log::Info() << "No previously existing manifest file to override from.";
        return;
    }

    const json jsonContents = json::parse(manifestFile);

    for (const auto& element : jsonContents)
    {
        if (element.is_object())
        {
            std::string path;
            std::string modeName;

            auto pathIt = element.find("path");
            auto modeIt = element.find("mode");
            if (pathIt != element.end() && pathIt->is_string() && modeIt != element.end() && modeIt->is_string())
            {
                path = pathIt->get<std::string>();
                modeName = modeIt->get<std::string>();

                if (modeName != magic_enum::enum_name(ManifestEntryMode::Background))
                {
                    auto it = m_Entries.find(path);
                    if (it != m_Entries.end())
                    {
                        auto mode = magic_enum::enum_cast<ManifestEntryMode>(modeName);
                        if (mode.has_value())
                        {
                            it->second->SetMode(mode.value());   
                        }
                    }
                }
            }
        }
    }
}

std::string Manifest::GenerateHash(const fs::path& file)
{
    std::ifstream ifs(file, std::ios::in | std::ios::binary | std::ios::ate);
    if (ifs.good())
    {
        std::ifstream::pos_type fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        std::vector<char> bytes;
        bytes.resize(static_cast<size_t>(fileSize));
        ifs.read(bytes.data(), fileSize);
        ifs.close();

        std::stringstream ss;
        ss << XXH3_64bits(bytes.data(), fileSize);
        return ss.str();
    }
    
    return std::string("");
}

} // namespace WingsOfSteel::Forge