#include <glm/gtc/matrix_transform.hpp>

#include <core/log.hpp>
#include <pandora.hpp>

#include "orbit_camera.hpp"

namespace WingsOfSteel::TheBrightestStar
{

OrbitCamera::OrbitCamera()
: m_Distance(10.0f)
, m_OrbitAngle(glm::radians(90.0f))
, m_Pitch(0.0f)
, m_MinimumPitch(glm::radians(0.0f))
, m_MaximumPitch(glm::radians(80.0f))
, m_RightMouseButtonPressedToken(Pandora::InputSystem::sInvalidInputCallbackToken)
, m_RightMouseButtonReleasedToken(Pandora::InputSystem::sInvalidInputCallbackToken)
, m_MousePositionToken(Pandora::InputSystem::sInvalidInputCallbackToken)
, m_Orbiting(false)
{
    CalculateCameraTransform();
}

OrbitCamera::~OrbitCamera()
{

}

void OrbitCamera::OnAddedToScene(Pandora::Scene* pScene)
{
    using namespace Pandora;
    m_RightMouseButtonPressedToken = GetInputSystem()->AddMouseButtonCallback([this](){ this->m_Orbiting = true; }, MouseButton::Right, MouseAction::Pressed);
    m_RightMouseButtonReleasedToken = GetInputSystem()->AddMouseButtonCallback([this](){ this->m_Orbiting = false; }, MouseButton::Right, MouseAction::Released);

    m_MousePositionToken = GetInputSystem()->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta){ 
        if (m_Orbiting)
        {
            const float sensitivity = 0.15f;
            m_OrbitAngle += glm::radians(mouseDelta.x * sensitivity);
            m_Pitch += glm::radians(mouseDelta.y * sensitivity);

            if (m_Pitch < m_MinimumPitch)
            {
                m_Pitch = m_MinimumPitch;
            }
            else if (m_Pitch > m_MaximumPitch)
            {
                m_Pitch = m_MaximumPitch;
            }
        }
    });
}

void OrbitCamera::OnRemovedFromScene()
{
    using namespace Pandora;
    GetInputSystem()->RemoveMouseButtonCallback(m_RightMouseButtonPressedToken);
    GetInputSystem()->RemoveMouseButtonCallback(m_RightMouseButtonReleasedToken);
    GetInputSystem()->RemoveMousePositionCallback(m_MousePositionToken);
}

void OrbitCamera::Update(float delta) 
{
    Pandora::Camera::Update(delta); 

    if (m_Orbiting)
    {
        CalculateCameraTransform();
    }
}

void OrbitCamera::CalculateCameraTransform()
{
    glm::vec3 position(
        glm::cos(m_OrbitAngle) * glm::cos(m_Pitch),
        glm::sin(m_Pitch),
        glm::sin(m_OrbitAngle) * glm::cos(m_Pitch)
    );

    //Pandora::Log::Info() << position.x << "," << position.y << "," << position.z << ", pitch " << glm::degrees(m_Pitch);

    LookAt(position * m_Distance, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));  
}

} // namespace WingsOfSteel::TheBrightestStar