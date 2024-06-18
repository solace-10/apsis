#pragma once

#include <functional>
#include <memory>
#include <string>

#include "vfs/file.hpp"

namespace WingsOfSteel::Pandora
{

namespace Private
{
    class VFSImpl;
}

enum class FileReadResult
{
    Ok,
    ErrorFileNotFound,
    ErrorHashMismatch,
    ErrorGeneric
};

using FileReadCallback = std::function<void(FileReadResult, File*)>;

class VFS
{
public:
    VFS();
    ~VFS();

    bool FileExists(const std::string& path) const;
    void FileRead(const std::string& path, FileReadCallback onFileReadCompleted);

private:
    std::unique_ptr<Private::VFSImpl> m_pImpl;
};

} // namespace WingsOfSteel::Pandora