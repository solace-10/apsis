#include <vector>

#include <pandora.hpp>

#include "components/metadata_component.hpp"
#include "components/mouse_picking_component.hpp"
#include "components/propagation_pending_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/camera_system.hpp"
#include "systems/space_object_picking_system.hpp"

namespace WingsOfSteel
{

SpaceObjectPickingSystem::~SpaceObjectPickingSystem()
{
    InputSystem* pInputSystem = GetInputSystem();
    if (pInputSystem)
    {
        pInputSystem->RemoveMouseButtonCallback(m_LeftMouseButtonPressedToken);
        pInputSystem->RemoveMouseButtonCallback(m_LeftMouseButtonReleasedToken);
        pInputSystem->RemoveMousePositionCallback(m_MousePositionToken);
    }
}

void SpaceObjectPickingSystem::Initialize(Scene* pScene)
{
    m_LeftMouseButtonPressedToken = GetInputSystem()->AddMouseButtonCallback(
        [this]() {
            m_PressPosition = m_CurrentMousePosition;
        },
        MouseButton::Left, MouseAction::Pressed);

    m_LeftMouseButtonReleasedToken = GetInputSystem()->AddMouseButtonCallback(
        [this]() {
            if (m_PressPosition.has_value())
            {
                const glm::vec2 delta = m_CurrentMousePosition - m_PressPosition.value();
                const float displacementSquared = glm::dot(delta, delta);
                const float dragDeadzoneSquared = CameraSystem::GetDragDeadzone() * CameraSystem::GetDragDeadzone();
                if (displacementSquared < dragDeadzoneSquared)
                {
                    PerformPick(m_CurrentMousePosition);
                }
                m_PressPosition.reset();
            }
        },
        MouseButton::Left, MouseAction::Released);

    m_MousePositionToken = GetInputSystem()->AddMousePositionCallback(
        [this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta) {
            m_CurrentMousePosition = mousePosition;
        });
}

void SpaceObjectPickingSystem::Update(float delta)
{
}

void SpaceObjectPickingSystem::PerformPick(const glm::vec2& screenPos)
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<MousePickingComponent, MetadataComponent>(entt::exclude<PropagationPendingComponent>);

    const float pickRadiusSquared = kPickRadiusPixels * kPickRadiusPixels;

    struct PickingData
    {
        EntityHandle entity;
        float z;  
    };

    std::vector<PickingData> entitiesInRadius;
    
    view.each([&](const EntityHandle entityHandle, const MousePickingComponent& mousePickingComponent, const MetadataComponent& metadataComponent) {
        if (!mousePickingComponent.IsEnabled())
        {
            return;
        }

        const glm::vec3 screenSpacePosition = mousePickingComponent.GetScreenSpacePosition();
        const glm::vec2 delta = glm::vec2(screenSpacePosition.x, screenSpacePosition.y) - screenPos;
        const float distanceSquared = glm::dot(delta, delta);
        if (distanceSquared < pickRadiusSquared)
        {
            entitiesInRadius.push_back({entityHandle, screenSpacePosition.z});
        }
    });

    EntityHandle closestEntity = NullEntityHandle;
    float closestDistance = 1.0f;
    for (auto& pickingData : entitiesInRadius)
    {
        if (pickingData.z <= closestDistance)
        {
            closestEntity = pickingData.entity;
            closestDistance = pickingData.z;
        }
    }

    if (closestEntity != NullEntityHandle)
    {
        pSector->SetSelectedSpaceObject(GetActiveScene()->GetEntity(closestEntity));
    }
    else
    {
        pSector->SetSelectedSpaceObject(nullptr);
    }
}

} // namespace WingsOfSteel
