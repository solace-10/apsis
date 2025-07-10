#include "systems/weapon_system.hpp"

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/weapon_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void WeaponSystem::Initialize(Pandora::Scene* pScene)
{
    // Initialization stub
}

void WeaponSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const WeaponComponent, TransformComponent>();

    view.each([delta, this](const auto entity, const WeaponComponent& weaponComponent, TransformComponent& transformComponent) {
        glm::mat4 shipWorldTransform{ 1.0f };
        EntitySharedPtr pShip = weaponComponent.m_pParent.lock();
        if (pShip)
        {
            shipWorldTransform = pShip->GetComponent<TransformComponent>().transform;
        }

        glm::mat4 hardpointWorldTransform = shipWorldTransform * weaponComponent.m_AttachmentPointTransform;
        const glm::vec3 hardpointTranslation(hardpointWorldTransform[3]);
        const glm::vec3 hardpointForward(-shipWorldTransform[2]); // We negate the forward vector because our ships are -Z facing.
        const glm::vec3 hardpointUp(shipWorldTransform[1]);

        DrawFiringArc(
            hardpointTranslation,
            hardpointForward,
            hardpointUp,
            weaponComponent.m_ArcMinDegrees,
            weaponComponent.m_ArcMaxDegrees,
            weaponComponent.m_Range);

        transformComponent.transform = hardpointWorldTransform;

        if (weaponComponent.m_Target)
        {
            GetDebugRender()->Line(hardpointTranslation, weaponComponent.m_Target.value(), Color::Red);
        }
    });
}

void WeaponSystem::DrawFiringArc(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float arcMinDegrees, float arcMaxDegrees, float arcLength)
{
    using namespace Pandora;

    // Draw base forward line
    // GetDebugRender()->Line(position, position + forward * arcLength, Color::Purple);

    // Draw arc min line
    glm::mat4 rotationMin = glm::rotate(glm::mat4(1.0f), glm::radians(arcMinDegrees), up);
    glm::vec3 rotatedForwardMin = glm::vec3(rotationMin * glm::vec4(forward, 0.0f));
    GetDebugRender()->Line(position, position + rotatedForwardMin * arcLength, Color::Yellow);

    // Draw arc max line
    glm::mat4 rotationMax = glm::rotate(glm::mat4(1.0f), glm::radians(arcMaxDegrees), up);
    glm::vec3 rotatedForwardMax = glm::vec3(rotationMax * glm::vec4(forward, 0.0f));
    GetDebugRender()->Line(position, position + rotatedForwardMax * arcLength, Color::Yellow);

    // Draw the arc edge as connected line segments
    glm::vec3 prevPoint = position;
    for (float angle = arcMinDegrees; angle <= arcMaxDegrees; angle += 1.0f)
    {
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), up);
        glm::vec3 rotatedForward = glm::vec3(rotation * glm::vec4(forward, 0.0f));
        glm::vec3 currentPoint = position + rotatedForward * arcLength;

        GetDebugRender()->Line(prevPoint, currentPoint, Color::Yellow);
        prevPoint = currentPoint;
    }
}

} // namespace WingsOfSteel::TheBrightestStar
