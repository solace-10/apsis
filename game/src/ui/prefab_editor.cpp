#include <imgui/imgui.hpp>

#include "ui/prefab_editor.hpp"
#include "game.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

PrefabEditor::PrefabEditor()
{

}

PrefabEditor::~PrefabEditor()
{

}

void PrefabEditor::Initialize()
{

}

void PrefabEditor::AddPrefabData(PrefabDataSharedPtr pPrefabData)
{
    m_PrefabData.push_back(pPrefabData);
}

void PrefabEditor::ShowPrefabEditor(bool state)
{
    m_ShowPrefabEditor = state;
}

void PrefabEditor::DrawPrefabEditor()
{
    if (!m_ShowPrefabEditor)
    {
        return;
    }

    ImGui::Begin("Prefab Editor", &m_ShowPrefabEditor);

    ImGui::BeginChild("PrefabList", ImVec2(300, 0), ImGuiChildFlags_Border);
    for (const auto& pPrefabDataWeakPtr : m_PrefabData)
    {
        PrefabDataSharedPtr pPrefabData = pPrefabDataWeakPtr.lock();
        if (pPrefabData)
        {
            if (ImGui::Selectable(pPrefabData->GetPath().c_str(), m_pSelectedPrefabData == pPrefabData))
            {
                m_pSelectedPrefabData = pPrefabData;
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("PrefabData", ImVec2(0, 0), ImGuiChildFlags_Border);
    if (m_pSelectedPrefabData)
    {
        ImGui::Button("Save");
        ImGui::SameLine();
        ImGui::Button("Revert");
        ImGui::Separator();
        const PrefabDataContainer& data = m_pSelectedPrefabData->GetData();
        for (const auto& [key, value] : data)
        {
            if (std::holds_alternative<int>(value))
            {
                int v = std::get<int>(value);
                ImGui::InputInt(key.c_str(), &v);
                m_pSelectedPrefabData->Set(key, v);
            }
            else if (std::holds_alternative<float>(value))
            {
                float v = std::get<float>(value);
                ImGui::InputFloat(key.c_str(), &v);
                m_pSelectedPrefabData->Set(key, v);
            }
            else if (std::holds_alternative<std::string>(value))
            {
                std::string v = std::get<std::string>(value);
                ImGui::InputText(key.c_str(), &v);
                m_pSelectedPrefabData->Set(key, v);
            }
            else
            {
                ImGui::Text("%s: unsupported type", key.c_str());
            }
        }
    }
    else
    {
        ImGui::Text("No prefab selected");
    }

    ImGui::EndChild();
    ImGui::End();
}

} // namespace WingsOfSteel::TheBrightestStar::UI