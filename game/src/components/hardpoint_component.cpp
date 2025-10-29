#include <resources/resource_system.hpp>
#include <pandora.hpp>

#include "components/hardpoint_component.hpp"

namespace WingsOfSteel
{

void Hardpoint::Deserialize(const ResourceDataStore* pContext, const nlohmann::json& json)
{
    m_Name = Json::DeserializeString(pContext, json, "name");
    m_Weapon = Json::DeserializeString(pContext, json, "weapon", "");
    m_ArcMinDegrees = Json::DeserializeFloat(pContext, json, "arc_min_degrees");
    m_ArcMaxDegrees = Json::DeserializeFloat(pContext, json, "arc_max_degrees");
    m_AutomatedTargeting = Json::DeserializeBool(pContext, json, "automated_targeting", true);
}

void HardpointComponent::Deserialize(const ResourceDataStore* pContext, const Json::Data& json)
{
    using namespace WingsOfSteel;

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
        hardpoint.Deserialize(pContext, hardpointJson);
        hardpoints.push_back(hardpoint);
    }

    m_ResourcePath = Json::DeserializeString(pContext, json, "resource");
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

} // namespace WingsOfSteel