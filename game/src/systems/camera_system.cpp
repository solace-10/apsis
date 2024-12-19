#include <scene/components/camera_component.hpp>
#include <scene/components/orbit_camera_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/entity.hpp>
#include <pandora.hpp>

#include "components/sub_sector_camera_component.hpp"
#include "systems/camera_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

CameraSystem::CameraSystem()
{

}
    
CameraSystem::~CameraSystem()
{
    Pandora::InputSystem* pInputSystem = Pandora::GetInputSystem();
    if (pInputSystem)
    {
        pInputSystem->RemoveMouseButtonCallback(m_RightMouseButtonPressedToken);
        pInputSystem->RemoveMouseButtonCallback(m_RightMouseButtonReleasedToken);
        pInputSystem->RemoveMousePositionCallback(m_MousePositionToken);
    }
}

void CameraSystem::Initialize()
{
    using namespace Pandora;
    m_RightMouseButtonPressedToken = GetInputSystem()->AddMouseButtonCallback([this](){ m_IsDragging = true; }, MouseButton::Right, MouseAction::Pressed);
    m_RightMouseButtonReleasedToken = GetInputSystem()->AddMouseButtonCallback([this](){ m_IsDragging = false; }, MouseButton::Right, MouseAction::Released);

    m_MousePositionToken = GetInputSystem()->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta){ 
        m_InputPending = true;
        m_MouseDelta = mouseDelta;
    });
}

void CameraSystem::Update(float delta)
{
    using namespace Pandora;
    EntitySharedPtr pCamera = GetActiveScene() ? GetActiveScene()->GetCamera(): nullptr;
    if (pCamera == nullptr)
    {
        return;
    }

    if (pCamera->HasComponent<CameraComponent>())
    {
        if (pCamera->HasComponent<SubSectorCameraComponent>())
        {
            SubSectorCameraComponent& subSectorCameraComponent = pCamera->GetComponent<SubSectorCameraComponent>();

            EntitySharedPtr pAnchorEntity = subSectorCameraComponent.GetAnchorEntity().lock();
            glm::vec3 anchorPosition(0.0f);
            if (pAnchorEntity && pAnchorEntity->HasComponent<TransformComponent>())
            {
                const glm::mat4& anchorTransform = pAnchorEntity->GetComponent<TransformComponent>().transform;
                anchorPosition = glm::vec3(anchorTransform[3]);
            }

            glm::vec3 offset = subSectorCameraComponent.GetOffset();
            CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
            cameraComponent.camera.LookAt(anchorPosition + offset, glm::vec3(0.0f, 0.0f, 1000.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (pCamera->HasComponent<OrbitCameraComponent>())
        {
            OrbitCameraComponent& orbitCameraComponent = pCamera->GetComponent<OrbitCameraComponent>();
            if (m_IsDragging && m_InputPending)
            {
                const float sensitivity = 0.15f;
                orbitCameraComponent.orbitAngle -= glm::radians(m_MouseDelta.x * sensitivity);
                orbitCameraComponent.pitch += glm::radians(m_MouseDelta.y * sensitivity);

                if (orbitCameraComponent.pitch < orbitCameraComponent.minimumPitch)
                {
                    orbitCameraComponent.pitch = orbitCameraComponent.minimumPitch;
                }
                else if (orbitCameraComponent.pitch > orbitCameraComponent.maximumPitch)
                {
                    orbitCameraComponent.pitch = orbitCameraComponent.maximumPitch;
                }
                m_InputPending = false;
            }

            glm::vec3 position(
                glm::cos(orbitCameraComponent.orbitAngle) * glm::cos(orbitCameraComponent.pitch),
                glm::sin(orbitCameraComponent.pitch),
                glm::sin(orbitCameraComponent.orbitAngle) * glm::cos(orbitCameraComponent.pitch)
            );

            EntitySharedPtr pAnchorEntity = orbitCameraComponent.anchorEntity.lock();
            glm::vec3 anchorPosition(0.0f);
            if (pAnchorEntity && pAnchorEntity->HasComponent<TransformComponent>())
            {
                const glm::mat4& anchorTransform = pAnchorEntity->GetComponent<TransformComponent>().transform;
                anchorPosition = glm::vec3(anchorTransform[3]);
            }

            CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
            cameraComponent.camera.LookAt(anchorPosition + position * orbitCameraComponent.distance, anchorPosition, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
}

} // namespace WingsOfSteel::TheBrightestStar