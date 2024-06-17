#if defined(TARGET_PLATFORM_NATIVE)

#include "vfs/private/native/vfs_native.hpp"

#include <cassert>
#include <fstream>
#include <filesystem>
#include <vector>

#include "vfs/file.hpp"

namespace WingsOfSteel::Pandora::Private
{

VFSNative::VFSNative()
{

}

VFSNative::~VFSNative()
{

}

bool VFSNative::FileExists(const std::string& path) const
{
    return std::filesystem::exists(path);
}
    
void VFSNative::FileRead(const std::string& path, FileReadCallback onFileReadCompleted)
{
    assert(onFileReadCompleted);

    if (!FileExists(path))
    {
        onFileReadCompleted(FileReadResult::ErrorFileNotFound, nullptr);
        return;
    }

    // std::ios::ate = immediately seek to the end of the stream.
    std::ifstream ifs(path, std::ios::in | std::ios::binary | std::ios::ate );

    if (ifs.good())
    {
        std::ifstream::pos_type fileSize = ifs.tellg();
        std::vector<char> bytes;
        bytes.reserve(static_cast<size_t>(fileSize));
        ifs.read(bytes.data(), fileSize);
        ifs.close();
        onFileReadCompleted(FileReadResult::Ok, new File(bytes));
    }
    else
    {
        onFileReadCompleted(FileReadResult::ErrorGeneric, nullptr);
    }
}

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_NATIVE