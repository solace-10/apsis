#pragma once

#include "scene/camera.hpp"

namespace WingsOfSteel::Pandora
{

class CameraComponent
{
public:
    CameraComponent(float fov, float nearPlane, float farPlane)
    : camera(fov, nearPlane, farPlane)
    {

    }

    ~CameraComponent() {}

    Camera camera;
};

} // namespace WingsOfSteel::Pandora