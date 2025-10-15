#include "systems/threat_indicator_system.hpp"

#include <pandora.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/threat_component.hpp"
#include "components/faction_component.hpp"

namespace WingsOfSteel
{

void ThreatIndicatorSystem::Initialize(Scene* pScene)
{
    // Initialization stub
}

void ThreatIndicatorSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const ThreatIndicatorComponent, const FactionComponent, const TransformComponent>();

    view.each([delta, this](
        const auto entity,
        const ThreatIndicatorComponent& threatIndicatorComponent,
        const FactionComponent& factionComponent,
        const TransformComponent& transformComponent) {

        // TODO: Implement threat indicator logic here
    });
}

} // namespace WingsOfSteel
