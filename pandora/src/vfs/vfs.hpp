#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

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

using FileReadCallback = std::function<void(FileReadResult, FileSharedPtr)>;

// The VFS provides a layer of abstraction over the underlying file system, as well as
// providing the foundation for mod support.
// Currently all files must exist under `data/core` but in the future mods could be supported,
// e.g. `data/mod1`, `data/mod2` etc for native system, which would overlay previously
// defined files.
// This would allow `data/mod1/ship1/diffuse.jpg` to override `data/core/ship1/diffuse.jpg`.
// The file would be access as `/ship1/diffuse.jpg`.
// Access to the overriden file is not possible.
// Note that with the current system, only native would support mods, with web relying on
// the predetermined manifest file in `data/core/manifest.json`, which is created by
// the `manifest_generator` tool.
class VFS
{
public:
    VFS();
    ~VFS();

    void Initialize();
    void Update();
    void FileRead(const std::string& path, FileReadCallback onFileReadCompleted);
    bool Exists(const std::string& path) const;
    const std::vector<std::string> List(const std::string& path = "/") const;

private:
    std::unique_ptr<Private::VFSImpl> m_pImpl;
};

} // namespace WingsOfSteel::Pandora