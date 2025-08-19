#include <resources/resource_system.hpp>
#include <pandora.hpp>

#include "components/hardpoint_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

nlohmann::json Hardpoint::Serialize() const
{
    nlohmann::json json;
    json["name"] = m_Name;
    json["weapon"] = m_Weapon;
    json["arc_min_degrees"] = m_ArcMinDegrees;
    json["arc_max_degrees"] = m_ArcMaxDegrees;
    return json;
}

void Hardpoint::Deserialize(const nlohmann::json& json)
{
    m_Name = Pandora::IComponent::DeserializeRequired<std::string>(json, "name");
    m_Weapon = Pandora::IComponent::DeserializeOptional<std::string>(json, "weapon", "");
    m_ArcMinDegrees = Pandora::IComponent::DeserializeRequired<float>(json, "arc_min_degrees");
    m_ArcMaxDegrees = Pandora::IComponent::DeserializeRequired<float>(json, "arc_max_degrees");
}

nlohmann::json HardpointComponent::Serialize() const
{
    nlohmann::json json;
    nlohmann::json hardpointsArray = nlohmann::json::array();
    
    for (const auto& hardpoint : hardpoints)
    {
        hardpointsArray.push_back(hardpoint.Serialize());
    }
    
    json["hardpoints"] = hardpointsArray;
    json["resource"] = m_ResourcePath;
    return json;
}

void HardpointComponent::Deserialize(const nlohmann::json& json)
{
    using namespace Pandora;

    hardpoints.clear();
    
    if (!json.contains("hardpoints"))
    {
        Log::Error() << "Missing required field: hardpoints";
        return;
    }
    
    const auto& hardpointsArray = json["hardpoints"];
    if (!hardpointsArray.is_array())
    {
        Log::Error() << "Hardpoints field must be an array";
        return;
    }
    
    for (const auto& hardpointJson : hardpointsArray)
    {
        Hardpoint hardpoint;
        hardpoint.Deserialize(hardpointJson);
        hardpoints.push_back(hardpoint);
    }

    m_ResourcePath = DeserializeRequired<std::string>(json, "resource");
    GetResourceSystem()->RequestResource(m_ResourcePath, [this](ResourceSharedPtr pResource) {
        m_pResource = std::dynamic_pointer_cast<ResourceModel>(pResource);

        for (auto& hardpoint : hardpoints)
        {
            auto attachmentPoint = m_pResource->GetAttachmentPoint(hardpoint.m_Name);
            if (attachmentPoint.has_value())
            {
                hardpoint.m_AttachmentPointTransform = attachmentPoint.value().m_ModelTransform;
            }
            else
            {
                Log::Warning() << "Failed to find attachment point '" << hardpoint.m_Name << "' in '" << m_ResourcePath << "'.";
            }
        }
    });
}

} // namespace WingsOfSteel::TheBrightestStar