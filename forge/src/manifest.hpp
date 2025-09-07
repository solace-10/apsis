#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <core/smart_ptr.hpp>

#include "manifest_entry.hpp"

namespace WingsOfSteel
{

class Manifest
{
public:
    Manifest();
    ~Manifest();

    const std::vector<ManifestEntry*> GetEntries() const;
    void Save();

private:
    void BuildFromFilesystem();
    void OverrideFromPreviousManifest();
    std::string GenerateHash(const std::filesystem::path& file);

    std::map<std::string, ManifestEntryUniquePtr> m_Entries;
};

} // namespace WingsOfSteel