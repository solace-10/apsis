#pragma once

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(Entity);

class OrbitCameraComponent
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
};

} // namespace WingsOfSteel::Pandora