#include "core/serialization.hpp"
#include "space_objects/space_object.hpp"

namespace WingsOfSteel
{

SpaceObject::SpaceObject()
{
}

SpaceObject::~SpaceObject()
{
}

bool SpaceObject::DeserializeOMM(const Json::Data& data)
{
    auto objectName = Json::TryDeserializeString(nullptr, data, "OBJECT_NAME");
    auto objectId = Json::TryDeserializeString(nullptr, data, "OBJECT_ID");
    auto epoch = Json::TryDeserializeString(nullptr, data, "EPOCH");
    auto meanMotion = Json::TryDeserializeFloat(nullptr, data, "MEAN_MOTION");
    auto eccentricity = Json::TryDeserializeFloat(nullptr, data, "ECCENTRICITY");
    auto inclination = Json::TryDeserializeFloat(nullptr, data, "INCLINATION");
    auto raOfAscNode = Json::TryDeserializeFloat(nullptr, data, "RA_OF_ASC_NODE");
    auto argOfPericenter = Json::TryDeserializeFloat(nullptr, data, "ARG_OF_PERICENTER");
    auto meanAnomaly = Json::TryDeserializeFloat(nullptr, data, "MEAN_ANOMALY");
    auto noradCatId = Json::TryDeserializeUnsignedInteger(nullptr, data, "NORAD_CAT_ID");

    if (!objectName.has_value() || !objectId.has_value() || !epoch.has_value() ||
        !meanMotion.has_value() || !eccentricity.has_value() || !inclination.has_value() ||
        !raOfAscNode.has_value() || !argOfPericenter.has_value() || !meanAnomaly.has_value() ||
        !noradCatId.has_value())
    {
        return false;
    }

    m_ObjectName = objectName.value();
    m_ObjectId = objectId.value();
    m_Epoch = epoch.value();
    m_MeanMotion = meanMotion.value();
    m_Eccentricity = eccentricity.value();
    m_Inclination = inclination.value();
    m_RightAscensionOfAscendingNode = raOfAscNode.value();
    m_ArgumentOfPericenter = argOfPericenter.value();
    m_MeanAnomaly = meanAnomaly.value();
    m_NoradCatalogueId = noradCatId.value();

    return true;
}

} // namespace WingsOfSteel
