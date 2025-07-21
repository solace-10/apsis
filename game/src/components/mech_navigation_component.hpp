#pragma once

#include <glm/vec3.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class MechNavigationComponent : public Pandora::IComponent
{
public:
    MechNavigationComponent() = default;
    ~MechNavigationComponent() = default;

    const glm::vec3& GetThrust() const { return m_Thrust; }
    void SetThrust(const glm::vec3& thrust) { m_Thrust = thrust; }

    const glm::vec3& GetAim() const { return m_Aim; }
    void SetAim(const glm::vec3& aim) { m_Aim = aim; }

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["thrust"] = SerializeVec3(m_Thrust);
        json["aim"] = SerializeVec3(m_Aim);
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        m_Thrust = DeserializeVec3(json, "thrust");
        m_Aim = DeserializeVec3(json, "aim");
    }

private:
    glm::vec3 m_Thrust{0.0f};
    glm::vec3 m_Aim{0.0f};
};

PANDORA_REGISTER_COMPONENT(MechNavigationComponent, "mech_navigation")

} // namespace WingsOfSteel::TheBrightestStar