#if defined(TARGET_PLATFORM_NATIVE)

#include "vfs/private/native/vfs_native.hpp"

#include <cassert>
#include <fstream>
#include <filesystem>
#include <vector>

#include "core/log.hpp"
#include "vfs/file.hpp"

namespace fs = std::filesystem;

namespace WingsOfSteel::Pandora::Private
{

VFSNative::VFSNative()
{
    BuildVFS();
    Log::Info() << "Native VFS initialized.";
}

VFSNative::~VFSNative()
{

}

void VFSNative::Initialize()
{

}

void VFSNative::Update()
{

}
    
void VFSNative::FileRead(const std::string& path, FileReadCallback onFileReadCompleted)
{
    assert(onFileReadCompleted);

    auto it = m_VFS.find(path);
    if (it == m_VFS.end())
    {
        onFileReadCompleted(FileReadResult::ErrorFileNotFound, nullptr);
        return;
    }

    // std::ios::ate = immediately seek to the end of the stream.
    std::ifstream ifs(it->second, std::ios::in | std::ios::binary | std::ios::ate );

    if (ifs.good())
    {
        std::ifstream::pos_type fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        std::vector<char> bytes;
        bytes.resize(static_cast<size_t>(fileSize));
        ifs.read(bytes.data(), fileSize);
        ifs.close();
        onFileReadCompleted(FileReadResult::Ok, std::make_shared<File>(path, std::move(bytes)));
    }
    else
    {
        onFileReadCompleted(FileReadResult::ErrorGeneric, nullptr);
    }
}

bool VFSNative::Exists(const std::string& path) const
{
    return m_VFS.find(path) != m_VFS.end();
}

void VFSNative::BuildVFS()
{
    // TODO: This can be extended to support modding.
    const std::string directory("data/core");
    if (fs::exists(directory) && fs::is_directory(directory)) 
    {
        const size_t prefixLength = directory.length();
        for (const auto& entry : fs::recursive_directory_iterator(directory)) 
        {
            if (entry.is_regular_file())
            {
                std::string relativePath(entry.path().string().substr(prefixLength));
                std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
                m_VFS[relativePath] = entry.path();
            }
        }
    }
    else
    {
        Log::Error() << "Failed to build VFS from '" << directory << "'. Incorrect working directory?";
    }
}

const std::vector<std::string> VFSNative::List(const std::string& path) const
{
    std::vector<std::string> files;
    for (auto& file : m_VFS)
    {
        if (file.first.starts_with(path))
        {
            files.push_back(file.first);
        }
    }
    return files;
}

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_NATIVE