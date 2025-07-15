#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include <core/log.hpp>

#include "components/ship_engine_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "systems/ship_navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void ShipNavigationSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const ShipNavigationComponent, const ShipEngineComponent, RigidBodyComponent>();

    view.each([delta](const auto entity, const ShipNavigationComponent& shipNavigationComponent, const ShipEngineComponent& shipEngineComponent, RigidBodyComponent& rigidBodyComponent) {
        if (shipNavigationComponent.GetThrust() != ShipThrust::None)
        {
            glm::vec3 force = rigidBodyComponent.GetForwardVector() * shipEngineComponent.linearForce;

            if (shipNavigationComponent.GetThrust() == ShipThrust::Backward)
            {
                force *= -0.5f;
            }

            rigidBodyComponent.ApplyLinearForce(force);
        }

        if (shipNavigationComponent.GetSteer() != ShipSteer::None)
        {
            const float dir = (shipNavigationComponent.GetSteer() == ShipSteer::Port) ? 1.0f : -1.0f;
            const float torque = shipEngineComponent.torque * dir;
            rigidBodyComponent.ApplyAngularForce(glm::vec3(0.0f, torque, 0.0f));
        }

        /*
        const glm::vec4 translation = rigidBodyComponent.GetWorldTransform()[3];
        const glm::vec3 pos(translation.x, translation.y, translation.z);
        Pandora::GetDebugRender()->Arrow(
            pos,
            pos + rigidBodyComponent.GetForwardVector() * 100.0f,
            Pandora::Color::Purple,
            10.0f);

        glm::vec3 lv = rigidBodyComponent.GetLinearVelocity();
        Pandora::Log::Info() << "lv: " << lv.x << " " << lv.y << " " << lv.z;
        glm::vec3 av = rigidBodyComponent.GetAngularVelocity();
        Pandora::Log::Info() << "av: " << av.x << " " << av.y << " " << av.z;
        */
    });
}

} // namespace WingsOfSteel::TheBrightestStar