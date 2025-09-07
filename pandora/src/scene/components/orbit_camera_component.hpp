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

    void Deserialize(const nlohmann::json& json) override
    {
        distance = DeserializeOptional<float>(json, "distance", 10.0f);
        orbitAngle = DeserializeOptional<float>(json, "orbit_angle", 0.0f);
        pitch = DeserializeOptional<float>(json, "pitch", 0.0f);
        minimumPitch = DeserializeOptional<float>(json, "minimum_pitch", 0.0f);
        maximumPitch = DeserializeOptional<float>(json, "maximum_pitch", 1.0f);
    }
};

REGISTER_COMPONENT(OrbitCameraComponent, "orbit_camera")

} // namespace WingsOfSteel