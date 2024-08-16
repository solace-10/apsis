#include <imgui.h>

#include "sector/sector_camera.hpp"
#include "sector/sector_info.hpp"
#include "sector/sector.hpp"
#include "sector/signal/signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Sector::Sector(SectorInfoSharedPtr& pSectorInfo)
: m_pSectorInfo(pSectorInfo)
, m_ShowSignalsDebugUI(false)
{

}

Sector::~Sector()
{

}

void Sector::Initialize()
{
    Pandora::Scene::Initialize();

    m_pCamera = std::make_shared<SectorCamera>();
    m_pCamera->LookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    AddEntity(m_pCamera);
    SetCamera(m_pCamera);

    for (auto& pSignal : m_pSectorInfo->GetSignals())
    {
        Pandora::EntitySharedPtr pEntity = pSignal->Spawn();
        AddEntity(pEntity);
    }

    DrawSignalsDebugUI();
}

void Sector::Update(float delta)
{
    Pandora::Scene::Update(delta);

    DrawSignalsDebugUI();
}

void Sector::ShowSignalsDebugUI(bool state)
{
    m_ShowSignalsDebugUI = state;
}

void Sector::DrawSignalsDebugUI()
{
    if (!m_ShowSignalsDebugUI)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(512, 512));
    ImGui::Begin("Signals", &m_ShowSignalsDebugUI);

    if (ImGui::BeginTable("table", 4, ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Difficulty");
        ImGui::TableSetupColumn("X");
        ImGui::TableSetupColumn("Z");
        ImGui::TableHeadersRow();

        for (auto& pSignal : m_pSectorInfo->GetSignals())
        {
            ImGui::TableNextRow();
            const glm::vec3& pos = pSignal->GetPosition();
            ImGui::TableNextColumn(); ImGui::Text("%s", pSignal->GetName().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.2f", pSignal->GetSignalDifficulty());
            ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(pos.x));
            ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(pos.z));
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

} // namespace WingsOfSteel::TheBrightestStar