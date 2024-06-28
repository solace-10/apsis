#include "vfs/file.hpp"

namespace WingsOfSteel::Pandora
{

File::File(const std::string& path, const FileData& data)
{
    m_Path = path;
    m_Data = data;
}

File::~File()
{

}

const FileData& File::GetData() const
{
    return m_Data;
}

const std::string& File::GetPath() const
{
    return m_Path;
}

} // namespace WingsOfSteel::Pandora