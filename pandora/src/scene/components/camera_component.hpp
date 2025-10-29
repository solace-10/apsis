#pragma once

#include "scene/camera.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel
{

class CameraComponent : public IComponent
{
public:
    CameraComponent(float fov, float nearPlane, float farPlane)
        : camera(fov, nearPlane, farPlane)
    {
    }

    CameraComponent() : camera(45.0f, 0.01f, 100.0f) {}
    ~CameraComponent() {}

    Camera camera;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        float fov = Json::DeserializeFloat(pContext, json, "fov", 45.0f);
        float nearPlane = Json::DeserializeFloat(pContext, json, "near_plane", 0.01f);
        float farPlane = Json::DeserializeFloat(pContext, json, "far_plane", 100.0f);

        camera.SetFieldOfView(fov);
        camera.SetNearPlane(nearPlane);
        camera.SetFarPlane(farPlane);

        glm::vec3 position = Json::DeserializeVec3(pContext, json, "position");
        glm::vec3 target = Json::DeserializeVec3(pContext, json, "target");
        camera.LookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }
};

REGISTER_COMPONENT(CameraComponent, "camera")

} // namespace WingsOfSteel