#pragma once

#include <optional>

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
    std::optional<glm::vec3> m_Target;
    Pandora::EntityWeakPtr m_pParent;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["attachment_point_transform"] = SerializeMat4(m_AttachmentPointTransform);
        json["arc_min_degrees"] = m_ArcMinDegrees;
        json["arc_max_degrees"] = m_ArcMaxDegrees;
        json["angle_degrees"] = m_AngleDegrees;
        json["range"] = m_Range;
        
        if (m_Target.has_value())
        {
            json["target"] = SerializeVec3(m_Target.value());
            json["has_target"] = true;
        }
        else
        {
            json["has_target"] = false;
        }
        
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        m_AttachmentPointTransform = DeserializeMat4(json, "attachment_point_transform");
        m_ArcMinDegrees = DeserializeRequired<float>(json, "arc_min_degrees");
        m_ArcMaxDegrees = DeserializeRequired<float>(json, "arc_max_degrees");
        m_AngleDegrees = DeserializeRequired<float>(json, "angle_degrees");
        m_Range = DeserializeOptional<float>(json, "range", 100.0f);
        
        bool hasTarget = DeserializeOptional<bool>(json, "has_target", false);
        if (hasTarget)
        {
            m_Target = DeserializeVec3(json, "target");
        }
        else
        {
            m_Target.reset();
        }
    }
};

REGISTER_COMPONENT(WeaponComponent, "weapon")

} // namespace WingsOfSteel::TheBrightestStar