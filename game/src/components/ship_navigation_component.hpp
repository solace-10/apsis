#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

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

class ShipNavigationComponent : public Pandora::IComponent
{
public:
    ShipNavigationComponent() {}
    ~ShipNavigationComponent() {}

    ShipThrust GetThrust() const { return m_Thrust; }
    void SetThrust(ShipThrust value) { m_Thrust = value; }
    ShipSteer GetSteer() const { return m_Steer; }
    void SetSteer(ShipSteer value) { m_Steer = value; }

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
    }

private:
    ShipThrust m_Thrust{ShipThrust::None};
    ShipSteer m_Steer{ShipSteer::None};
};

REGISTER_COMPONENT(ShipNavigationComponent, "ship_navigation")

} // namespace WingsOfSteel::TheBrightestStar