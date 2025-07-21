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
        json["attachmentPointTransform"] = SerializeMat4(m_AttachmentPointTransform);
        json["arcMinDegrees"] = m_ArcMinDegrees;
        json["arcMaxDegrees"] = m_ArcMaxDegrees;
        json["angleDegrees"] = m_AngleDegrees;
        json["range"] = m_Range;
        
        if (m_Target.has_value())
        {
            json["target"] = SerializeVec3(m_Target.value());
            json["hasTarget"] = true;
        }
        else
        {
            json["hasTarget"] = false;
        }
        
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        m_AttachmentPointTransform = DeserializeMat4(json, "attachmentPointTransform");
        m_ArcMinDegrees = DeserializeRequired<float>(json, "arcMinDegrees");
        m_ArcMaxDegrees = DeserializeRequired<float>(json, "arcMaxDegrees");
        m_AngleDegrees = DeserializeRequired<float>(json, "angleDegrees");
        m_Range = DeserializeOptional<float>(json, "range", 100.0f);
        
        bool hasTarget = DeserializeOptional<bool>(json, "hasTarget", false);
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

PANDORA_REGISTER_COMPONENT_WITH_ENTITY_INTEGRATION(WeaponComponent, "weapon")

} // namespace WingsOfSteel::TheBrightestStar