#pragma once

#include <optional>

#include <glm/vec3.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class MechNavigationComponent : public IComponent
{
public:
    MechNavigationComponent() = default;
    ~MechNavigationComponent() = default;

    const std::optional<glm::vec3>& GetThrust() const { return m_Thrust; }
    void SetThrust(const glm::vec3& thrust) { m_Thrust = thrust; }
    void ClearThrust() { m_Thrust.reset(); }

    const std::optional<glm::vec3>& GetAim() const { return m_Aim; }
    void SetAim(const glm::vec3& aim) { m_Aim = aim; }
    void ClearAim() { m_Aim.reset(); }

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
    }

private:
    std::optional<glm::vec3> m_Thrust;
    std::optional<glm::vec3> m_Aim;
};

REGISTER_COMPONENT(MechNavigationComponent, "mech_navigation")

} // namespace WingsOfSteel