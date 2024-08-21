#pragma once

namespace WingsOfSteel::TheBrightestStar
{

enum class ShipSteerCommand
{
    None,
    Port,
    Starboard
};

class ShipNavigationComponent
{
public:
    ShipNavigationComponent() {}
    ~ShipNavigationComponent() {}

    ShipSteerCommand GetSteerCommand() const;
    void SetSteerCommand(ShipSteerCommand command);
    float GetThrust() const;
    void SetThrust(float value);

private:
    ShipSteerCommand m_SteerCommand = ShipSteerCommand::None;
    float m_Thrust = 0.0f;
};

inline ShipSteerCommand ShipNavigationComponent::GetSteerCommand() const
{
    return m_SteerCommand;
}

inline void ShipNavigationComponent::SetSteerCommand(ShipSteerCommand command)
{
    m_SteerCommand = command;
}

inline float ShipNavigationComponent::GetThrust() const
{
    return m_Thrust;
}

inline void ShipNavigationComponent::SetThrust(float value)
{
    m_Thrust = value;
}

} // namespace WingsOfSteel::TheBrightestStar