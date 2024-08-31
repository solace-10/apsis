#pragma once

#include <stdint.h>
#include <string>

namespace WingsOfSteel::Pandora
{

enum class WindowMode
{
    Windowed,
    Fullscreen
};

class WindowSettings
{
public:
    WindowSettings() {}
    ~WindowSettings() {}

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const;
    void SetSize(uint32_t width, uint32_t height);
    void GetSize(uint32_t& width, uint32_t& height) const;
    void SetMode(WindowMode mode);
    WindowMode GetMode() const;

private:
    std::string m_Title{ "Pandora" };
    uint32_t m_Width{ 1024 };
    uint32_t m_Height{ 768 };
    WindowMode m_Mode{ WindowMode::Windowed };
};

inline void WindowSettings::SetTitle(const std::string& title)
{
    m_Title = title;
}

inline const std::string& WindowSettings::GetTitle() const
{
    return m_Title;
}

inline void WindowSettings::SetSize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
}

inline void WindowSettings::GetSize(uint32_t& width, uint32_t& height) const
{
    width = m_Width;
    height = m_Height;
}

inline void WindowSettings::SetMode(WindowMode mode)
{
    m_Mode = mode;
}

inline WindowMode WindowSettings::GetMode() const
{
    return m_Mode;
}

} // namespace WingsOfSteel::Pandora