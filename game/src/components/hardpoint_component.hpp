#pragma once

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

struct Hardpoint
{
    std::string m_Name;
    glm::mat4 m_AttachmentPointTransform{ 1.0f };
    float m_ArcMinDegrees{ 0.0f };
    float m_ArcMaxDegrees{ 0.0f };
    float m_AngleDegrees{ 0.0f };
    Pandora::EntitySharedPtr m_pEntity;
    Pandora::EntityWeakPtr m_pParent;
    
    nlohmann::json Serialize() const
    {
        nlohmann::json json;
        json["name"] = m_Name;
        json["attachmentPointTransform"] = Pandora::IComponent::SerializeMat4(m_AttachmentPointTransform);
        json["arcMinDegrees"] = m_ArcMinDegrees;
        json["arcMaxDegrees"] = m_ArcMaxDegrees;
        json["angleDegrees"] = m_AngleDegrees;
        return json;
    }
    
    void Deserialize(const nlohmann::json& json)
    {
        m_Name = Pandora::IComponent::DeserializeRequired<std::string>(json, "name");
        m_AttachmentPointTransform = Pandora::IComponent::DeserializeMat4(json, "attachmentPointTransform");
        m_ArcMinDegrees = Pandora::IComponent::DeserializeRequired<float>(json, "arcMinDegrees");
        m_ArcMaxDegrees = Pandora::IComponent::DeserializeRequired<float>(json, "arcMaxDegrees");
        m_AngleDegrees = Pandora::IComponent::DeserializeRequired<float>(json, "angleDegrees");
    }
};

class HardpointComponent : public Pandora::IComponent
{
public:
    HardpointComponent() {}
    std::vector<Hardpoint> hardpoints;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        nlohmann::json hardpointsArray = nlohmann::json::array();
        
        for (const auto& hardpoint : hardpoints)
        {
            hardpointsArray.push_back(hardpoint.Serialize());
        }
        
        json["hardpoints"] = hardpointsArray;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        hardpoints.clear();
        
        if (!json.contains("hardpoints"))
        {
            Pandora::Log::Error() << "Missing required field: hardpoints";
            throw std::runtime_error("Missing required field: hardpoints");
        }
        
        const auto& hardpointsArray = json["hardpoints"];
        if (!hardpointsArray.is_array())
        {
            Pandora::Log::Error() << "Hardpoints field must be an array";
            throw std::runtime_error("Hardpoints field must be an array");
        }
        
        for (const auto& hardpointJson : hardpointsArray)
        {
            Hardpoint hardpoint;
            hardpoint.Deserialize(hardpointJson);
            hardpoints.push_back(hardpoint);
        }
    }
};
PANDORA_REGISTER_COMPONENT_WITH_ENTITY_INTEGRATION(HardpointComponent, "hardpoint")

} // namespace WingsOfSteel::TheBrightestStar