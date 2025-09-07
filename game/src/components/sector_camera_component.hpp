#pragma once

#include <glm/vec3.hpp>

#include <core/smart_ptr.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{
    DECLARE_SMART_PTR(Entity);
}

namespace WingsOfSteel
{

class SectorCameraComponent : public IComponent
{
public:
    SectorCameraComponent() {}
    ~SectorCameraComponent() {}

    glm::vec3 defaultOffset{0.0f, 60.0f, 45.0f};
    glm::vec3 position{0.0f};
    glm::vec3 maximumDrift{0.0f};
    float driftTimer{0.0f};
    glm::vec3 target{0.0f};
    glm::vec3 positionVelocity{0.0f};
    glm::vec3 targetVelociy{0.0f};
    float backOffFactor{0.0f};
    float backOffFactorVelocity{0.0f};
    glm::vec3 aimLocal{0.0f};
    glm::vec3 aimLocalVelocity{0.0f};
    EntityWeakPtr anchorEntity;
    bool debugDraw{false};

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["position"] = SerializeVec3(position);
        json["maximum_drift"] = SerializeVec3(maximumDrift);
        json["drift_timer"] = driftTimer;
        json["target"] = SerializeVec3(target);
        json["position_velocity"] = SerializeVec3(positionVelocity);
        json["target_velocity"] = SerializeVec3(targetVelociy);
        json["debug_draw"] = debugDraw;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        position = DeserializeVec3(json, "position");
        maximumDrift = DeserializeVec3(json, "maximum_drift");
        driftTimer = DeserializeRequired<float>(json, "drift_timer");
        target = DeserializeVec3(json, "target");
        positionVelocity = DeserializeVec3(json, "position_velocity");
        targetVelociy = DeserializeVec3(json, "target_velocity");
        debugDraw = DeserializeOptional<bool>(json, "debug_draw", false);
    }
};

REGISTER_COMPONENT(SectorCameraComponent, "sector_camera")

} // namespace WingsOfSteel