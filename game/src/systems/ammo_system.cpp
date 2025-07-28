#include "systems/ammo_system.hpp"

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/ammo_raycast_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "game.hpp"
#include "sector/sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void AmmoSystem::Initialize(Pandora::Scene* pScene)
{
    // Initialization stub
}

void AmmoSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<TransformComponent, const AmmoRaycastComponent>();

    view.each([delta, this](const auto entity, TransformComponent& transformComponent, const AmmoRaycastComponent& ammoRaycastComponent) {
        
        glm::vec3 startPos = transformComponent.GetTranslation();
        glm::vec3 direction = -transformComponent.GetForward();
        glm::vec3 endPos = startPos + direction * ammoRaycastComponent.GetRaycastLength();

        GetDebugRender()->Line(
            startPos,
            endPos,
            Color::Red
        );

        const glm::vec3 offsetPos(startPos + direction * delta * ammoRaycastComponent.GetSpeed());
        transformComponent.transform[3] = glm::vec4(offsetPos.x, offsetPos.y, offsetPos.z, 1.0f);

        /*
        GetDebugRender()->Circle(
            transformComponent.GetTranslation(),
            glm::vec3(0.0f, 1.0f, 0.0f),
            Color::White,
            0.5f,
            8.0f
        );*/
    });
}

void AmmoSystem::Instantiate(Pandora::EntitySharedPtr pWeaponEntity, const WeaponComponent& weaponComponent)
{
    using namespace Pandora;

    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    glm::mat4 rootWorldTransform{ 1.0f };
    EntitySharedPtr pParentEntity = pWeaponEntity->GetParent().lock();
    if (pParentEntity)
    {
        rootWorldTransform = pParentEntity->GetComponent<TransformComponent>().transform;
    }
    const glm::mat4 hardpointWorldTransform = rootWorldTransform * weaponComponent.m_AttachmentPointTransform;
    // TODO: This should include the muzzle transform.

    Pandora::SceneWeakPtr pWeakScene = pSector->GetWeakPtr();
    EntityBuilder::Build(
        pWeakScene,
        weaponComponent.m_Ammo,
        hardpointWorldTransform,
        [](Pandora::EntitySharedPtr pEntity)
        {

        }
    );
}

} // namespace WingsOfSteel::TheBrightestStar
