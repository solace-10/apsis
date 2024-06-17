#pragma once

#include <memory>
#include <vector>

namespace WingsOfSteel::Pandora
{

class File;
using FileWeakptr = std::weak_ptr<File>;
using FileSharedPtr = std::shared_ptr<File>;
using FileData = std::vector<char>;

class File
{
public:
    File(const FileData& data);
    ~File();

    const FileData& GetData() const;

private:
    FileData m_Data;
};

} // namespace WingsOfSteel::Pandora