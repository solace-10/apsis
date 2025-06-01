#pragma once

#include <string>

namespace WingsOfSteel::Pandora
{

class TagComponent
{
public:
    TagComponent(const std::string& value)
        : m_Tag(value)
    {
    }

    ~TagComponent() {}

    const std::string& Value() const;

private:
    std::string m_Tag;
};

inline const std::string& TagComponent::Value() const
{
    return m_Tag;
}

} // namespace WingsOfSteel::Pandora