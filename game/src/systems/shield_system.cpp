#include <glm/gtx/rotate_vector.hpp>

#include "systems/shield_system.hpp"

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/ghost_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "components/faction_component.hpp"
#include "components/shield_component.hpp"

namespace WingsOfSteel
{

ShieldSystem::~ShieldSystem()
{
}

void ShieldSystem::Initialize(Scene* pScene)
{
}

void ShieldSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<ShieldComponent, TransformComponent, const FactionComponent>();
    view.each([delta, this](const auto entity, ShieldComponent& shieldComponent, TransformComponent& transformComponent, const FactionComponent& factionComponent) {
        glm::mat4 rootWorldTransform{ 1.0f };
        EntitySharedPtr pShieldEntity = shieldComponent.GetOwner().lock();
        if (!pShieldEntity)
        {
            return;
        }

        EntitySharedPtr pParentEntity = pShieldEntity->GetParent().lock();
        if (pParentEntity)
        {
            rootWorldTransform = pParentEntity->GetComponent<TransformComponent>().transform;
            transformComponent.transform = rootWorldTransform;
        }
    });
}

} // namespace WingsOfSteel
