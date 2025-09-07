#pragma once

#include <optional>

#include <glm/vec3.hpp>

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

class ShipNavigationComponent : public Pandora::IComponent
{
public:
    ShipNavigationComponent() {}
    ~ShipNavigationComponent() {}

    ShipThrust GetThrust() const { return m_Thrust; }
    void SetThrust(ShipThrust value) { m_Thrust = value; }
    void SetTarget(const glm::vec3& position) { m_Target = position; }
    void ClearTarget() { m_Target.reset(); }
    const std::optional<glm::vec3>& GetTarget() const { return m_Target; }

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
    std::optional<glm::vec3> m_Target;
};

REGISTER_COMPONENT(ShipNavigationComponent, "ship_navigation")

} // namespace WingsOfSteel::TheBrightestStar