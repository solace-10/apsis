#include "systems/threat_indicator_system.hpp"

#include <pandora.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/threat_component.hpp"
#include "components/faction_component.hpp"

namespace WingsOfSteel
{

void ThreatIndicatorSystem::Update(float delta)
{
    m_CurrentUpdate++;
    
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const ThreatComponent, const FactionComponent, const TransformComponent>();

    view.each([this, delta](
        const auto entityHandle,
        const ThreatComponent& threatIndicatorComponent,
        const FactionComponent& factionComponent,
        const TransformComponent& transformComponent) {

        if (factionComponent.Value != Faction::Hostile)
        {
            return;
        }

        auto threatIt = m_Threats.find(entityHandle);
        if (threatIt == m_Threats.cend())
        {
            Threat threat;
            threat.timeSinceLastSeen = 0.0f;
            threat.lastUpdate = m_CurrentUpdate;
            threat.position = transformComponent.transform[3];
            m_Threats[entityHandle] = threat;
            return;
        }

        Threat& threat = threatIt->second;
        threat.timeSinceSpawned += delta;
        threat.lastUpdate = m_CurrentUpdate;
        threat.position = transformComponent.transform[3];
    });

    static const float sRemovalDelay = 1.0f; // Time in seconds before a threat which has disappeared is removed from the list.
    for (auto& threatIt : m_Threats)
    {
        Threat& threat = threatIt.second;
        if (threat.lastUpdate != m_CurrentUpdate)
        {
            threat.timeSinceLastSeen += delta;
        }
    }

    std::erase_if(m_Threats, [](const auto& item) {
        return item.second.timeSinceLastSeen > sRemovalDelay;
    });
}

std::vector<Threat> ThreatIndicatorSystem::GetThreats() const
{
    std::vector<Threat> threats;

    for (const auto& threat : m_Threats)
    {
        threats.push_back(threat.second);
    }

    return threats;    
}

} // namespace WingsOfSteel
