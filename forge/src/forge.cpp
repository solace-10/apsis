#include <sstream>

#include <magic_enum.hpp>

#include <core/log.hpp>
#include <imgui/imgui_system.hpp>
#include <pandora.hpp>

#include "forge.hpp"
#include "manifest.hpp"

namespace WingsOfSteel
{

Forge::Forge()
{

}

Forge::~Forge()
{

}

void Forge::Initialize()
{
    Log::Info() << "Initializing Forge...";
    GetImGuiSystem()->SetGameMenuBarCallback([this](){ DrawImGuiMenuBar(); });
    m_pManifest = std::make_unique<Manifest>();
}

void Forge::Update(float delta)
{
    using namespace ImGui;
    const float menubarSize = 18.0f;
    SetNextWindowPos(ImVec2(0, menubarSize));

    ImVec2 size = GetIO().DisplaySize;
    size.y -= menubarSize;

    SetNextWindowSize(size, ImGuiCond_Always);
    int flags = 0;
    flags |= ImGuiWindowFlags_AlwaysAutoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoTitleBar;
    Begin("Forge", nullptr, flags);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Forge", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Manifest"))
        {
            DrawManifestTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Web cache"))
        {
            ImGui::Text("This is the Broccoli tab!\nblah blah blah blah blah");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    End();
}

void Forge::Shutdown()
{

}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Forge::DrawImGuiMenuBar()
{

}

void Forge::DrawManifestTab()
{
    if (ImGui::BeginTable("table", 3, ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("Path");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Embedded");
        ImGui::TableHeadersRow();

        if (m_pManifest)
        {
            auto modeLabelFromIndex = [](int idx)
            {
                ManifestEntryMode mode = static_cast<ManifestEntryMode>(idx);
                return magic_enum::enum_name(mode).data();
            };

            int comboIdx = 0;
            for (auto& pEntry : m_pManifest->GetEntries())
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%s", pEntry->GetPath().data());
                ImGui::TableNextColumn(); ImGui::Text("%lu", pEntry->GetSize());
                ImGui::TableNextColumn();

                int itemCurrentIdx = static_cast<int>(pEntry->GetMode());
                std::stringstream comboId;
                comboId << "##combo_" << comboIdx++;
                ImGui::SetNextItemWidth(128.0f);

                if (ImGui::BeginCombo(comboId.str().data(), modeLabelFromIndex(itemCurrentIdx)))
                {
                    for (int n = 0; n < magic_enum::enum_count<ManifestEntryMode>(); n++)
                    {
                        const bool isSelected = (itemCurrentIdx == n);
                        if (ImGui::Selectable(modeLabelFromIndex(n), isSelected))
                        {
                            itemCurrentIdx = n;
                            pEntry->SetMode(static_cast<ManifestEntryMode>(itemCurrentIdx));
                            m_pManifest->Save();
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }
        ImGui::EndTable();
    }
}

} // namespace WingsOfSteel