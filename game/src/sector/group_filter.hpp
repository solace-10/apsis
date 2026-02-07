#pragma once

#include <cstdint>
#include <string>

namespace WingsOfSteel
{

class GroupFilter
{
public:
    GroupFilter(const std::string& name, const std::string& displayName, const std::string& color, bool enabled, uint8_t bitIndex)
    : m_Name(name)
    , m_DisplayName(displayName)
    , m_Color(color)
    , m_Enabled(enabled)
    , m_BitIndex(bitIndex)
    {}

    ~GroupFilter() = default;

    const std::string& GetName() const { return m_Name; }
    const std::string& GetDisplayName() const { return m_DisplayName; }
    const std::string& GetColor() const { return m_Color; }
    void SetCount(uint32_t count) { m_Count = count; }
    uint32_t GetCount() const { return m_Count; }
    void SetEnabled(bool isEnabled) { m_Enabled = isEnabled; }
    bool IsEnabled() const { return m_Enabled; }
    uint8_t GetBitIndex() const { return m_BitIndex; }

private:
    std::string m_Name;
    std::string m_DisplayName;
    std::string m_Color;
    uint32_t m_Count{ 0 };
    bool m_Enabled{ false };
    uint8_t m_BitIndex{ 0 };
};
    
} // namespace WingsOfSteel
