#pragma once

#include "core/smart_ptr.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Entity);

class OrbitCameraComponent : public IComponent
{
public:
    OrbitCameraComponent() {}
    ~OrbitCameraComponent() {}

    float distance = 10.0f;
    float orbitAngle = 0.0f;
    float pitch = 0.0f;
    float minimumPitch = 0.0f;
    float maximumPitch = 1.0f;
    EntityWeakPtr anchorEntity;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["distance"] = distance;
        json["orbit_angle"] = orbitAngle;
        json["pitch"] = pitch;
        json["minimum_pitch"] = minimumPitch;
        json["maximum_pitch"] = maximumPitch;
        return json;
    }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        distance = Json::DeserializeFloat(pContext, json, "distance", 10.0f);
        orbitAngle = Json::DeserializeFloat(pContext, json, "orbit_angle", 0.0f);
        pitch = Json::DeserializeFloat(pContext, json, "pitch", 0.0f);
        minimumPitch = Json::DeserializeFloat(pContext, json, "minimum_pitch", 0.0f);
        maximumPitch = Json::DeserializeFloat(pContext, json, "maximum_pitch", 1.0f);
    }
};

REGISTER_COMPONENT(OrbitCameraComponent, "orbit_camera")

} // namespace WingsOfSteel