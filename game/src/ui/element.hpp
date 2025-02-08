#pragma once

#include <string>

#include <glm/vec2.hpp>

#include <core/smart_ptr.hpp>

#include "ui/prefab_data.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Element);
class Element
{
public:
    Element() {}
    virtual ~Element() {}

    virtual void Render() {}

    void SetX(float x);
    void SetY(float y);
    void SetWidth(float width);
    void SetHeight(float height);
    void SetPosition(const glm::vec2& position);
    void SetSize(const glm::vec2& size);

    const glm::vec2& GetPosition() const;
    const glm::vec2& GetSize() const;

private:
    glm::vec2 m_Position{0.0f, 0.0f};
    glm::vec2 m_Size{512.0f, 512.0f};
};

inline void Element::SetX(float x)
{
    m_Position.x = x;
}

inline void Element::SetY(float y)
{
    m_Position.y = y;
}

inline void Element::SetWidth(float width)
{
    m_Size.x = width;
}

inline void Element::SetHeight(float height)
{
    m_Size.y = height;
}

inline void Element::SetPosition(const glm::vec2& position)
{
    m_Position = position;
}

inline void Element::SetSize(const glm::vec2& size)
{
    m_Size = size;
}

inline const glm::vec2& Element::GetPosition() const
{
    return m_Position;
}

inline const glm::vec2& Element::GetSize() const
{
    return m_Size;
}

} // namespace WingsOfSteel::TheBrightestStar::UI