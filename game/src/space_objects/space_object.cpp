#include <ctime>
#include <iomanip>
#include <sstream>

#include "core/serialization.hpp"
#include "space_objects/space_object.hpp"

namespace WingsOfSteel
{

namespace
{

// Parse ISO 8601 timestamp: "2026-01-14T15:04:24.142944"
std::chrono::system_clock::time_point ParseEpoch(const std::string& epochStr)
{
    std::tm tm = {};
    std::istringstream ss(epochStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    // Convert to time_t (UTC)
#ifdef _WIN32
    std::time_t time = _mkgmtime(&tm);
#else
    std::time_t time = timegm(&tm);
#endif

    auto tp = std::chrono::system_clock::from_time_t(time);

    // Parse fractional seconds if present
    if (ss.peek() == '.')
    {
        ss.get(); // consume the '.'
        std::string fractional;
        ss >> fractional;
        // Pad or truncate to 6 digits (microseconds)
        fractional.resize(6, '0');
        int microseconds = std::stoi(fractional);
        tp += std::chrono::microseconds(microseconds);
    }

    return tp;
}

} // anonymous namespace

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
    m_Epoch = ParseEpoch(epoch.value());
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
