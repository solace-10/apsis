#pragma once

#include "scene/camera.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel::Pandora
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

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["fov"] = camera.GetFieldOfView();
        json["near_plane"] = camera.GetNearPlane();
        json["far_plane"] = camera.GetFarPlane();
        json["position"] = SerializeVec3(camera.GetPosition());
        json["target"] = SerializeVec3(camera.GetTarget());
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        float fov = DeserializeOptional<float>(json, "fov", 45.0f);
        float nearPlane = DeserializeOptional<float>(json, "near_plane", 0.01f);
        float farPlane = DeserializeOptional<float>(json, "far_plane", 100.0f);
        
        camera.SetFieldOfView(fov);
        camera.SetNearPlane(nearPlane);
        camera.SetFarPlane(farPlane);
        
        glm::vec3 position = DeserializeVec3(json, "position");
        glm::vec3 target = DeserializeVec3(json, "target");
        camera.LookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }
};

REGISTER_COMPONENT(CameraComponent, "camera")

} // namespace WingsOfSteel::Pandora