#pragma once

namespace WingsOfSteel::TheBrightestStar
{

class ShipComponent
{
public:
    ShipComponent() {}
    ~ShipComponent() {}

    const std::string& GetName() const;
    void SetName(const std::string& name);

private:
    std::string m_Name;
};

inline const std::string& ShipComponent::GetName() const
{
    return m_Name;
}

inline void ShipComponent::SetName(const std::string& name)
{
    m_Name = name;
}

} // namespace WingsOfSteel::TheBrightestStar