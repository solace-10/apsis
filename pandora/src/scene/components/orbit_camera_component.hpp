#pragma once

namespace WingsOfSteel::Pandora
{

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
};

} // namespace WingsOfSteel::Pandora