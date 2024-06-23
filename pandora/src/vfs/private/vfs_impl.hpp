#pragma once

#include "vfs/vfs.hpp"

namespace WingsOfSteel::Pandora::Private
{

class VFSImpl
{
public:
    VFSImpl() {}
    virtual ~VFSImpl() {}
    
    virtual void Initialize() = 0;
    virtual void Update() = 0;
    virtual void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) = 0;
};

} // namespace WingsOfSteel::Pandora::Private