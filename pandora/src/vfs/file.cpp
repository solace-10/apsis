#include "vfs/file.hpp"

namespace WingsOfSteel::Pandora
{

File::File(const FileData& data)
{
    m_Data = data;
}

File::~File()
{

}

const FileData& File::GetData() const
{
    return m_Data;
}

} // namespace WingsOfSteel::Pandora