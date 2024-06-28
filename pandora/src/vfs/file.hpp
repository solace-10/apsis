#pragma once

#include <memory>
#include <string>
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
    File(const std::string& path, const FileData& data);
    ~File();

    const std::string& GetPath() const;
    const FileData& GetData() const;

private:
    std::string m_Path;
    FileData m_Data;
};

} // namespace WingsOfSteel::Pandora