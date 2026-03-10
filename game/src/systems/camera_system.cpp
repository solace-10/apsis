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
#include "systems/camera_system.hpp"

namespace WingsOfSteel
{

CameraSystem::~CameraSystem()
{
    InputSystem* pInputSystem = GetInputSystem();
    if (pInputSystem)
    {
        pInputSystem->RemoveMouseButtonCallback(m_LeftMouseButtonPressedToken);
        pInputSystem->RemoveMouseButtonCallback(m_LeftMouseButtonReleasedToken);
        pInputSystem->RemoveMousePositionCallback(m_MousePositionToken);
        pInputSystem->RemoveMouseWheelCallback(m_MouseWheelToken);
    }
}

void CameraSystem::Initialize(Scene* pScene)
{
    m_LeftMouseButtonPressedToken = GetInputSystem()->AddMouseButtonCallback(
        [this]() {
            m_IsButtonHeld = true;
            m_PressPosition = m_CurrentMousePosition;
        },
        MouseButton::Left, MouseAction::Pressed);

    m_LeftMouseButtonReleasedToken = GetInputSystem()->AddMouseButtonCallback(
        [this]() {
            m_IsButtonHeld = false;
            m_IsDragging = false;
        },
        MouseButton::Left, MouseAction::Released);

    m_MousePositionToken = GetInputSystem()->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta) {
        m_CurrentMousePosition = mousePosition;
        m_InputPending = true;
        m_MouseDelta = mouseDelta;
    });

    m_MouseWheelToken = GetInputSystem()->AddMouseWheelCallback([this](const glm::vec2& scroll) {
        m_ScrollDelta += scroll.y;
    });
}

void CameraSystem::Update(float delta)
{
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
            glm::vec3 cameraWantedTarget = sectorCameraComponent.target;
            glm::vec3 cameraWantedPosition = sectorCameraComponent.position;
            if (pAnchorEntity && pAnchorEntity->HasComponent<TransformComponent>())
            {
                const glm::mat4& anchorTransform = pAnchorEntity->GetComponent<TransformComponent>().transform;
                anchorPosition = glm::vec3(anchorTransform[3]);
                cameraWantedPosition = sectorCameraComponent.position + anchorPosition;
                cameraWantedTarget = anchorPosition;
            }

            DampSpring(sectorCameraComponent.position, cameraWantedPosition, sectorCameraComponent.positionVelocity, 0.5f, delta);
            sectorCameraComponent.target = cameraWantedTarget;

            CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
            cameraComponent.camera.LookAt(sectorCameraComponent.position, sectorCameraComponent.target, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (pCamera->HasComponent<OrbitCameraComponent>())
        {
            OrbitCameraComponent& occ = pCamera->GetComponent<OrbitCameraComponent>();

            if (m_IsButtonHeld && !m_IsDragging && m_InputPending)
            {
                const glm::vec2 displacement = m_CurrentMousePosition - m_PressPosition;
                const float displacementSquared = glm::dot(displacement, displacement);
                const float dragDeadzoneSquared = GetDragDeadzone() * GetDragDeadzone();
                if (displacementSquared >= dragDeadzoneSquared)
                {
                    m_IsDragging = true;
                }
            }

            if (m_IsDragging && m_InputPending)
            {
                // Resolution-independent deltas, normalized by window height.
                const float windowHeight = static_cast<float>(GetWindow()->GetHeight());
                const float normalizedDeltaX = m_MouseDelta.x / windowHeight;
                const float normalizedDeltaY = m_MouseDelta.y / windowHeight;

                // Calculate raw input velocity (radians per second)
                const glm::vec2 rawInputVelocity(
                    -normalizedDeltaX * occ.sensitivity / delta,
                    normalizedDeltaY * occ.sensitivity / delta);

                // Apply exponential moving average to smooth velocity
                m_SmoothedInputVelocity = glm::mix(m_SmoothedInputVelocity, rawInputVelocity, m_InputSmoothingFactor);

                // Apply smoothed velocity to wanted angles
                occ.wantedOrbitAngle += m_SmoothedInputVelocity.x * delta;
                occ.wantedPitch += m_SmoothedInputVelocity.y * delta;

                // Store for momentum
                m_OrbitAngleInputVelocity = m_SmoothedInputVelocity.x;
                m_PitchInputVelocity = m_SmoothedInputVelocity.y;

                m_InputPending = false;
            }
            else
            {
                // Decay smoothed velocity when not dragging (momentum)
                m_SmoothedInputVelocity *= m_MomentumDecay;

                if (glm::length(m_SmoothedInputVelocity) > 0.001f)
                {
                    occ.wantedOrbitAngle += m_SmoothedInputVelocity.x * delta;
                    occ.wantedPitch += m_SmoothedInputVelocity.y * delta;
                }
            }

            if (std::abs(m_ScrollDelta) > 0.01f)
            {
                occ.wantedDistance -= m_ScrollDelta * occ.zoomSensitivity;
                m_ScrollDelta = 0.0f;
            }

            occ.wantedPitch = glm::clamp(occ.wantedPitch, occ.minimumPitch, occ.maximumPitch);
            occ.wantedDistance = glm::clamp(occ.wantedDistance, occ.minimumDistance, occ.maximumDistance);

            DampSpring(occ.orbitAngle, occ.wantedOrbitAngle, occ.orbitAngleVelocity, occ.orbitAngleDamping, delta);
            DampSpring(occ.pitch, occ.wantedPitch, occ.pitchVelocity, occ.pitchDamping, delta);
            DampSpring(occ.distance, occ.wantedDistance, occ.distanceVelocity, occ.distanceDamping, delta);

            const glm::vec3 position(
                glm::cos(occ.orbitAngle) * glm::cos(occ.pitch),
                glm::sin(occ.pitch),
                glm::sin(occ.orbitAngle) * glm::cos(occ.pitch));

            CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
            cameraComponent.camera.LookAt(
                occ.anchorPosition + position * occ.distance,
                occ.anchorPosition,
                glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
}

glm::vec3 CameraSystem::MouseToWorld(const glm::vec2& mousePos) const
{
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

} // namespace WingsOfSteel
