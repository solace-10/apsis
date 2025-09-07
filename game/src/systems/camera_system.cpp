#include <numeric>

#include <core/interpolation.hpp>
#include <core/log.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <render/window.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/orbit_camera_component.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/entity.hpp>

#include "components/sector_camera_component.hpp"
#include "components/mech_navigation_component.hpp"
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

void CameraSystem::Initialize(Pandora::Scene* pScene)
{
    using namespace Pandora;
    m_RightMouseButtonPressedToken = GetInputSystem()->AddMouseButtonCallback([this]() { m_IsDragging = true; }, MouseButton::Right, MouseAction::Pressed);
    m_RightMouseButtonReleasedToken = GetInputSystem()->AddMouseButtonCallback([this]() { m_IsDragging = false; }, MouseButton::Right, MouseAction::Released);

    m_MousePositionToken = GetInputSystem()->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta) {
        m_InputPending = true;
        m_MouseDelta = mouseDelta;
    });
}

void CameraSystem::Update(float delta)
{
    using namespace Pandora;
    EntitySharedPtr pCamera = GetActiveScene() ? GetActiveScene()->GetCamera() : nullptr;
    if (pCamera == nullptr)
    {
        return;
    }

    if (pCamera->HasComponent<CameraComponent>())
    {
        if (pCamera->HasComponent<SectorCameraComponent>())
        {
            SectorCameraComponent& sectorCameraComponent = pCamera->GetComponent<SectorCameraComponent>();

            EntitySharedPtr pAnchorEntity = sectorCameraComponent.anchorEntity.lock();
            glm::vec3 anchorPosition(0.0f);
            glm::vec3 cameraWantedTarget(0.0f);
            glm::vec3 cameraWantedPosition = sectorCameraComponent.position;
            if (pAnchorEntity && pAnchorEntity->HasComponent<TransformComponent>())
            {
                const glm::mat4& anchorTransform = pAnchorEntity->GetComponent<TransformComponent>().transform;
                anchorPosition = glm::vec3(anchorTransform[3]);
                cameraWantedPosition = sectorCameraComponent.position + anchorPosition;
                cameraWantedTarget = anchorPosition;

                if (pAnchorEntity->HasComponent<MechNavigationComponent>() && pAnchorEntity->HasComponent<RigidBodyComponent>())
                {
                    const MechNavigationComponent& mechNavigationComponent = pAnchorEntity->GetComponent<MechNavigationComponent>();
                    const RigidBodyComponent& rigidBodyComponent = pAnchorEntity->GetComponent<RigidBodyComponent>();

                    const std::optional<glm::vec3>& mechAim = mechNavigationComponent.GetAim();
                    if (mechAim.has_value())
                    {
                        // Slowly move the camera back when the player is aiming towards the bottom of the screen (towards positive Z).
                        glm::vec3 mechAimTarget(mechAim.value());
                        glm::vec3 mechAimDirection = glm::normalize(mechAimTarget - anchorPosition);
                        const float backOffFactorTarget = glm::max(0.0f, mechAimDirection.z);
                        Pandora::DampSpring(sectorCameraComponent.backOffFactor, backOffFactorTarget, sectorCameraComponent.backOffFactorVelocity, 3.0f, delta);
                        glm::vec3 cameraBackoffOffset(0.0f, 0.0f, 30.0f * sectorCameraComponent.backOffFactor);

                        // Keep the camera target between the mech and where the player is aiming at.
                        // The aiming point is kept close to the mech to avoid having the camera turn too much,
                        // as that makes the mech difficult to control.
                        const float mechAimTargetDistance = glm::length(mechAimTarget - anchorPosition);
                        const float maxMechAimTargetDistance = 20.0f;
                        glm::vec3 mechAimTargetRestricted(mechAimTarget);
                        if (mechAimTargetDistance > maxMechAimTargetDistance)
                        {
                            mechAimTargetRestricted = anchorPosition + glm::normalize(mechAimTarget - anchorPosition) * maxMechAimTargetDistance;
                        }

                        const glm::vec3 wantedAimLocal = mechAimTargetRestricted - anchorPosition;
                        Pandora::DampSpring(sectorCameraComponent.aimLocal, wantedAimLocal, sectorCameraComponent.aimLocalVelocity, 2.0f, delta);

                        cameraWantedPosition = anchorPosition + sectorCameraComponent.defaultOffset + cameraBackoffOffset;
                        cameraWantedTarget = anchorPosition + sectorCameraComponent.aimLocal;
                    }
                    else
                    {
                        cameraWantedPosition = anchorPosition + sectorCameraComponent.defaultOffset;
                    }
                }
            }

            Pandora::DampSpring(sectorCameraComponent.position, cameraWantedPosition, sectorCameraComponent.positionVelocity, 0.5f, delta);

            sectorCameraComponent.position = sectorCameraComponent.position;
            sectorCameraComponent.target = cameraWantedTarget;

            CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
            cameraComponent.camera.LookAt(sectorCameraComponent.position, sectorCameraComponent.target, glm::vec3(0.0f, 1.0f, 0.0f));
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
                glm::sin(orbitCameraComponent.orbitAngle) * glm::cos(orbitCameraComponent.pitch));

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


glm::vec3 CameraSystem::MouseToWorld(const glm::vec2& mousePos) const
{
    using namespace Pandora;
    EntitySharedPtr pCamera = GetActiveScene() ? GetActiveScene()->GetCamera() : nullptr;
    if (pCamera == nullptr || !pCamera->HasComponent<CameraComponent>())
    {
        return glm::vec3(0.0f);
    }

    const CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
    const Camera& camera = cameraComponent.camera;

    // Get camera position and create ray direction
    const glm::vec3 cameraPos = camera.GetPosition();

    // Convert mouse to world coordinates at two different depths to create a ray
    const glm::vec3 nearPoint = camera.ScreenToWorld(mousePos, GetWindow()->GetWidth(), GetWindow()->GetHeight(), 0.0f);
    const glm::vec3 farPoint = camera.ScreenToWorld(mousePos, GetWindow()->GetWidth(), GetWindow()->GetHeight(), 1.0f);

    // Calculate ray direction
    const glm::vec3 rayDir = glm::normalize(farPoint - nearPoint);

    // Define the XZ plane (Y = 0)
    const glm::vec3 planeNormal(0.0f, 1.0f, 0.0f); // Up vector
    const glm::vec3 planePoint(0.0f, 0.0f, 0.0f); // Origin point on the plane

    // Perform ray-plane intersection
    const float denom = glm::dot(rayDir, planeNormal);

    // Check if ray is parallel to plane
    if (std::abs(denom) < std::numeric_limits<float>::epsilon())
    {
        return glm::vec3(0.0f); // No intersection
    }

    // Calculate intersection parameter
    const float t = glm::dot(planePoint - nearPoint, planeNormal) / denom;

    // Check if intersection is behind the camera
    if (t < 0.0f)
    {
        return glm::vec3(0.0f); // Intersection behind camera
    }

    // Calculate intersection point
    return nearPoint + rayDir * t;
}

} // namespace WingsOfSteel::TheBrightestStar