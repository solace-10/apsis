#include <sstream>

#include <magic_enum.hpp>

#include <imgui/imgui.hpp>
#include <pandora.hpp>

#include "components/carrier_component.hpp"
#include "components/faction_component.hpp"
#include "components/name_component.hpp"
#include "components/wing_component.hpp"
#include "sector/sector.hpp"
#include "systems/ai_strategic_system.hpp"
#include "game.hpp"

namespace WingsOfSteel
{

void AIStrategicSystem::Update(float delta)
{
    DrawDebugUI();

    m_NextUpdate -= delta;
    if (m_NextUpdate > 0.0f)
    {
        return;
    }

    m_NextUpdate = 1.0f;

    entt::registry& registry = GetActiveScene()->GetRegistry();

    auto controllerView = registry.view<CarrierComponent>();
    controllerView.each([this, delta](const auto entity, CarrierComponent& carrierComponent) {
        
    });
}

void AIStrategicSystem::DrawDebugUI()
{
    if (!IsDebugUIVisible())
    {
        return;
    }

    if (ImGui::Begin("Strategic layer", &m_ShowDebugUI))
    {
        // One header per carrier. 
        entt::registry& registry = GetActiveScene()->GetRegistry();
        auto carrierView = registry.view<CarrierComponent, const FactionComponent>();
        carrierView.each([this](const auto entity, CarrierComponent& carrierComponent, const FactionComponent& factionComponent) {
            std::stringstream carrierTitle;
            carrierTitle << "Carrier (";
            if (factionComponent.Value == Faction::Allied)
            {
                carrierTitle << "allied";
            }
            else
            {
                carrierTitle << "hostile";
            }
            carrierTitle << ")";
            if (ImGui::CollapsingHeader(carrierTitle.str().c_str()))
            {
                ImGui::Text("Offense: %.2f / %.2f", carrierComponent.CurrentRolePercentages[static_cast<size_t>(WingRole::Offense)], carrierComponent.TargetRolePercentages[static_cast<size_t>(WingRole::Offense)]);
                ImGui::Text("Defense: %.2f / %.2f", carrierComponent.CurrentRolePercentages[static_cast<size_t>(WingRole::Defense)], carrierComponent.TargetRolePercentages[static_cast<size_t>(WingRole::Defense)]);
                ImGui::Text("Intercept: %.2f / %.2f", carrierComponent.CurrentRolePercentages[static_cast<size_t>(WingRole::Interception)], carrierComponent.TargetRolePercentages[static_cast<size_t>(WingRole::Interception)]);

                if (ImGui::BeginTable("escorts", 3, ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("Escort");
                    ImGui::TableSetupColumn("Wing");
                    ImGui::TableSetupColumn("Behaviour");
                    ImGui::TableHeadersRow();

                    const Faction carrierFaction = factionComponent.Value;
                    auto escortView = GetActiveScene()->GetRegistry().view<const WingComponent, const FactionComponent, const NameComponent>();
                    escortView.each([this, carrierFaction](const auto entity, const WingComponent& wingComponent, const FactionComponent& factionComponent, const NameComponent& nameComponent) {
                        if (factionComponent.Value == carrierFaction)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", nameComponent.Value.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("%d", wingComponent.ID);
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", std::string(magic_enum::enum_name<WingRole>(wingComponent.Role)).c_str());
                        }
                    });

                    ImGui::EndTable();
                }
            }
        });

        ImGui::End();
    }
}

} // namespace WingsOfSteel