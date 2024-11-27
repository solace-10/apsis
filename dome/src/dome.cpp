#include <sstream>

#include <magic_enum.hpp>

#include <core/log.hpp>
#include <imgui/imgui_system.hpp>
#include <pandora.hpp>

#include "buffered_texture_2d.hpp"
#include "dome.hpp"

namespace WingsOfSteel::Dome
{

Dome::Dome()
{

}

Dome::~Dome()
{

}

void Dome::Initialize()
{
    using namespace Pandora;

    Log::Info() << "Initializing Dome...";
    GetImGuiSystem()->SetGameMenuBarCallback([this](){ DrawImGuiMenuBar(); });

    m_pSourceTexture = std::make_unique<BufferedTexture2D>("Dome source texture");
    if (!m_pSourceTexture->Load("data/core/source/nebula_1.png"))
    {
        Log::Error() << "Failed to load texture.";
    }
    else
    {
        m_EdgePoints = GetEdgePoints(m_Threshold);
    }
}

void Dome::Update(float delta)
{
    using namespace ImGui;
    const float menubarSize = 24.0f;
    SetNextWindowPos(ImVec2(0, menubarSize));

    ImVec2 size = GetIO().DisplaySize;
    size.y -= menubarSize;

    SetNextWindowSize(size, ImGuiCond_Always);
    int flags = 0;
    flags |= ImGuiWindowFlags_AlwaysAutoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoTitleBar;
    Begin("Dome", nullptr, flags);

    // Left
    static int selected = 0;
    {
        ImGui::BeginChild("left pane", ImVec2(300, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
        for (int i = 0; i < 100; i++)
        {
            // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
            char label[128];
            sprintf(label, "MyObject %d", i);
            if (ImGui::Selectable(label, selected == i))
                selected = i;
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();

    // Right
    {
        ImGui::BeginChild("item view", ImVec2(1024, 1024), 0, ImGuiWindowFlags_NoScrollbar); // Leave room for 1 line below us

        if (m_pSourceTexture)
        {
            ImTextureID textureId = m_pSourceTexture->GetTextureView().Get();
            const float textureWidth = 1024.0f;
            const float textureHeight = 1024.0f;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 uvMin = ImVec2(0.0f, 0.0f);
            ImVec2 uvMax = ImVec2(1.0f, 1.0f);
            ImVec4 tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
            ImVec4 borderCol = ImGui::GetStyleColorVec4(ImGuiCol_Border);
            ImVec2 c = ImGui::GetCursorScreenPos();
            ImGui::Image(textureId, ImVec2(textureWidth, textureHeight), uvMin, uvMax, tintCol, borderCol);

            ImDrawList* pDrawList = ImGui::GetWindowDrawList();
            ImU32 pointColor = ImGui::GetColorU32(IM_COL32(0, 255, 0, 255));
            int pointRadius = 1;

            for (auto& point : m_EdgePoints)
            {
                ImVec2 p1 = ImVec2(c.x + point.x - pointRadius, c.y + point.y - pointRadius);
                ImVec2 p2 = ImVec2(c.x + point.x + pointRadius, c.y + point.y - pointRadius);
                ImVec2 p3 = ImVec2(c.x + point.x + pointRadius, c.y + point.y + pointRadius);
                ImVec2 p4 = ImVec2(c.x + point.x - pointRadius, c.y + point.y + pointRadius);
                pDrawList->AddQuadFilled(p1, p2, p3, p4, pointColor);
            }
        }

        ImGui::EndChild();
    }

    End();
}

void Dome::Shutdown()
{

}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Dome::DrawImGuiMenuBar()
{

}

std::vector<Point> Dome::GetEdgePoints(int threshold) const
{
    size_t dataSize;
    int width, height, channels;
    unsigned char* pData = m_pSourceTexture->GetData(dataSize, width, height, channels);

    const int multiplier = 2;
    std::vector<Point> points;

    for (int y = 0; y < height; y += multiplier) {
        for (int x = 0; x < width; x += multiplier) {
            int sum = 0;
            int total = 0;

            for (int row = -1; row <= 1; row++) {
                int sy = y + row;
                int step = sy * width;

                if (sy >= 0 && sy < height) {
                    for (int col = -1; col <= 1; col++) {
                        int sx = x + col;

                        if (sx >= 0 && sx < width) {
                            sum += pData[(sx + step) * channels]; // << 2 is equivalent to * 4 for byte addressing
                            total++;
                        }
                    }
                }
            }

            if (total > 0) {
                sum /= total;
            }

            if (sum > threshold) {
                points.push_back({x, y});
            }
        }
    }

    return points;
}

} // namespace WingsOfSteel::Dome