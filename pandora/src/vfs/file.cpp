#include "vfs/file.hpp"

namespace WingsOfSteel::Pandora
{

File::File(const std::string& path, const FileData& data)
{
    m_Path = path;
    m_Data = data;

    size_t token = path.find_first_of(".");
    if (token != std::string::npos)
    {
        m_Extension = m_Path.substr(token + 1);
    }
}

File::~File()
{

}

const FileData& File::GetData() const
{
    return m_Data;
}

const std::string& File::GetExtension() const
{
    return m_Extension;
}

const std::string& File::GetPath() const
{
    return m_Path;
}

} // namespace WingsOfSteel::Pandora