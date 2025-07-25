#pragma once

#include <optional>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class WeaponComponent : public Pandora::IComponent
{
public:
    WeaponComponent() = default;

    glm::mat4 m_AttachmentPointTransform{ 1.0f };
    float m_ArcMinDegrees{ 0.0f };
    float m_ArcMaxDegrees{ 0.0f };
    float m_AngleDegrees{ 0.0f };
    float m_Range{ 100.0f };
    std::string m_Ammo;

    std::optional<glm::vec3> m_Target;
    Pandora::EntityWeakPtr m_pParent;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["range"] = m_Range;
        json["ammo"] = m_Ammo;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        m_Range = DeserializeRequired<float>(json, "range");
        m_Ammo = DeserializeRequired<std::string>(json, "ammo");
    }
};

REGISTER_COMPONENT(WeaponComponent, "weapon")

} // namespace WingsOfSteel::TheBrightestStar