#pragma once

#include <glm/vec3.hpp>

#include <core/smart_ptr.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::Pandora
{
    DECLARE_SMART_PTR(Entity);
}

namespace WingsOfSteel::TheBrightestStar
{

class SectorCameraComponent : public Pandora::IComponent
{
public:
    SectorCameraComponent() {}
    ~SectorCameraComponent() {}

    glm::vec3 position{0.0f};
    glm::vec3 maximumDrift{0.0f};
    float driftTimer{0.0f};
    glm::vec3 target{0.0f};
    glm::vec3 velocity{0.0f};
    Pandora::EntityWeakPtr anchorEntity;
    bool debugDraw{false};

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["position"] = SerializeVec3(position);
        json["maximumDrift"] = SerializeVec3(maximumDrift);
        json["driftTimer"] = driftTimer;
        json["target"] = SerializeVec3(target);
        json["velocity"] = SerializeVec3(velocity);
        json["debugDraw"] = debugDraw;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        position = DeserializeVec3(json, "position");
        maximumDrift = DeserializeVec3(json, "maximumDrift");
        driftTimer = DeserializeRequired<float>(json, "driftTimer");
        target = DeserializeVec3(json, "target");
        velocity = DeserializeVec3(json, "velocity");
        debugDraw = DeserializeOptional<bool>(json, "debugDraw", false);
    }
};

PANDORA_REGISTER_COMPONENT_WITH_ENTITY_INTEGRATION(SectorCameraComponent, "sector_camera")

} // namespace WingsOfSteel::TheBrightestStar