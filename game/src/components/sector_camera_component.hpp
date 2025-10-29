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

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        position = Json::DeserializeVec3(pContext, json, "position");
        maximumDrift = Json::DeserializeVec3(pContext, json, "maximum_drift");
        driftTimer = Json::DeserializeFloat(pContext, json, "drift_timer");
        target = Json::DeserializeVec3(pContext, json, "target");
        positionVelocity = Json::DeserializeVec3(pContext, json, "position_velocity");
        targetVelociy = Json::DeserializeVec3(pContext, json, "target_velocity");
        debugDraw = Json::DeserializeBool(pContext, json, "debug_draw", false);
    }
};

REGISTER_COMPONENT(SectorCameraComponent, "sector_camera")

} // namespace WingsOfSteel