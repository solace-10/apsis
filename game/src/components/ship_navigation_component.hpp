#pragma once

namespace WingsOfSteel::TheBrightestStar
{

enum class ShipThrust
{
    None,
    Forward,
    Backward
};

enum class ShipSteer
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

    ShipThrust GetThrust() const { return m_Thrust; }
    void SetThrust(ShipThrust value) { m_Thrust = value; }
    ShipSteer GetSteer() const { return m_Steer; }
    void SetSteer(ShipSteer value) { m_Steer = value; }

private:
    ShipThrust m_Thrust{ShipThrust::None};
    ShipSteer m_Steer{ShipSteer::None};
};

} // namespace WingsOfSteel::TheBrightestStar