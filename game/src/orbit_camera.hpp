#pragma once

#include <input/input_system.hpp>
#include <scene/camera.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(OrbitCamera);
class OrbitCamera : public Pandora::Camera
{
public:
    OrbitCamera();
    ~OrbitCamera();

    void OnAddedToScene(Pandora::Scene* pScene) override;
    void OnRemovedFromScene() override;
    void Update(float delta) override;

private:
    void CalculateCameraTransform();

    float m_Distance;
    float m_OrbitAngle;
    float m_Pitch;
    float m_MinimumPitch;
    float m_MaximumPitch;
    Pandora::InputCallbackToken m_RightMouseButtonPressedToken;
    Pandora::InputCallbackToken m_RightMouseButtonReleasedToken;
    Pandora::InputCallbackToken m_MousePositionToken;
    bool m_Orbiting;
};

} // namespace WingsOfSteel::TheBrightestStar