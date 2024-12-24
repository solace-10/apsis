#include <sstream>

#include <magic_enum.hpp>

#include <core/log.hpp>
#include <imgui/imgui_system.hpp>
#include <pandora.hpp>

#include "buffered_texture_2d.hpp"
#include "dome.hpp"
#include "geometry_processor.hpp"
#include "texture_processor.hpp"

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

    m_pTextureProcessor = std::make_unique<TextureProcessor>();
    m_pGeometryProcessor = std::make_unique<GeometryProcessor>();

    m_pSourceTexture = std::make_unique<BufferedTexture2D>("Dome source texture");
    if (!m_pSourceTexture->Load("data/core/source/nebula_1.png"))
    {
        Log::Error() << "Failed to load texture.";
    }
    else
    {
        m_pGreyscaleTexture = m_pTextureProcessor->GetGreyscale(m_pSourceTexture.get());
        m_pEdgeTexture = m_pTextureProcessor->GetEdges(m_pGreyscaleTexture.get());
        CalculateEdgePoints();
        BuildGeometry();
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
    static int selectedTextureIndex = 0; // Here we store our selection data as an index.
    {
        ImGui::BeginChild("left pane", ImVec2(500, 0), ImGuiChildFlags_Border);

        const char* items[] = { "Original", "Greyscale", "Edges" };

        // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
        const char* combo_preview_value = items[selectedTextureIndex];

        if (ImGui::BeginCombo("Texture", combo_preview_value, 0))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (selectedTextureIndex == n);
                if (ImGui::Selectable(items[n], is_selected))
                {
                    selectedTextureIndex = n;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Checkbox("Show edge points", &m_ShowEdgePoints);

        bool rebuildGeometry = false;
        if (ImGui::SliderInt("Edge threshold", &m_EdgePointThreshold, 10, 60))
        {
            CalculateEdgePoints();
            rebuildGeometry = true;
        }

        ImGui::Checkbox("Show edge vertices", &m_ShowEdgeVertices);

        if (ImGui::InputInt("Max vertex count", &m_MaxVertexCount, 100))
        {
            rebuildGeometry = true;
        }

        if (ImGui::SliderFloat("Vertex accuracy", &m_Accuracy, 0.0f, 1.0f))
        {
            rebuildGeometry = true;
        }

        ImGui::Checkbox("Show wireframes", &m_ShowWireframes);

        if (ImGui::Checkbox("Collapse vertices", &m_CollapseVertices))
        {
            rebuildGeometry = true;
        }

        if (ImGui::SliderFloat("Collapse threshold", &m_CollapseThreshold, 0.0f, 32.0f))
        {
            rebuildGeometry = true;
        }

        ImGui::Checkbox("Show mesh", &m_ShowMesh);

        if (rebuildGeometry)
        {
            BuildGeometry();
        }

        ImGui::EndChild();
    }
    ImGui::SameLine();

    // Right
    {
        ImGui::BeginChild("item view", ImVec2(1024, 1024), 0, ImGuiWindowFlags_NoScrollbar); // Leave room for 1 line below us

        BufferedTexture2D* pSelectedTexture = nullptr;
        switch (selectedTextureIndex)
        {
            case 0: pSelectedTexture = m_pSourceTexture.get(); break;
            case 1: pSelectedTexture = m_pGreyscaleTexture.get(); break;
            case 2: pSelectedTexture = m_pEdgeTexture.get(); break;
        };

        if (pSelectedTexture)
        {
            ImTextureID textureId = pSelectedTexture->GetTextureView().Get();
            const float textureWidth = 1024.0f;
            const float textureHeight = 1024.0f;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 uvMin = ImVec2(0.0f, 0.0f);
            ImVec2 uvMax = ImVec2(1.0f, 1.0f);
            ImVec2 c = ImGui::GetCursorScreenPos();
            ImGui::Image(textureId, ImVec2(textureWidth, textureHeight), uvMin, uvMax);

            ImDrawList* pDrawList = ImGui::GetWindowDrawList();

            if (m_ShowEdgePoints)
            {
                const ImU32 pointColor = ImGui::GetColorU32(IM_COL32(0, 255, 0, 255));
                const int pointRadius = 1;
                for (auto& point : m_EdgePoints)
                {
                    ImVec2 p1 = ImVec2(c.x + point.x - pointRadius, c.y + point.y - pointRadius);
                    ImVec2 p2 = ImVec2(c.x + point.x + pointRadius, c.y + point.y - pointRadius);
                    ImVec2 p3 = ImVec2(c.x + point.x + pointRadius, c.y + point.y + pointRadius);
                    ImVec2 p4 = ImVec2(c.x + point.x - pointRadius, c.y + point.y + pointRadius);
                    pDrawList->AddQuadFilled(p1, p2, p3, p4, pointColor);
                }
            }

            if (m_ShowMesh)
            {
                for (auto& triangle : m_IndexedTriangles)
                {
                    const Vertex& v0 = m_ColorizedVertices[triangle.v0];
                    const Vertex& v1 = m_ColorizedVertices[triangle.v1];
                    const Vertex& v2 = m_ColorizedVertices[triangle.v2];

                    ImVec2 p1 = ImVec2(c.x + v0.x, c.y + v0.y);
                    ImVec2 p2 = ImVec2(c.x + v1.x, c.y + v1.y);
                    ImVec2 p3 = ImVec2(c.x + v2.x, c.y + v2.y);
                    
                    ImU32 c1 = ImGui::GetColorU32(ImVec4(v0.color.r, v0.color.g, v0.color.b, 1.0f));
                    ImU32 c2 = ImGui::GetColorU32(ImVec4(v1.color.r, v1.color.g, v1.color.b, 1.0f));
                    ImU32 c3 = ImGui::GetColorU32(ImVec4(v2.color.r, v2.color.g, v2.color.b, 1.0f));

                    pDrawList->AddTriangleFilledMultiColor(
                        p1, p2, p3,
                        c1, c2, c3
                    );
                }
            }

            if (m_ShowWireframes)
            {
                const ImU32 triangleColor = ImGui::GetColorU32(IM_COL32(0, 0, 255, 255));
                for (auto& triangle : m_IndexedTriangles)
                {
                    ImVec2 p1 = ImVec2(c.x + m_EdgeVertices[triangle.v0].x, c.y + m_EdgeVertices[triangle.v0].y);
                    ImVec2 p2 = ImVec2(c.x + m_EdgeVertices[triangle.v1].x, c.y + m_EdgeVertices[triangle.v1].y);
                    ImVec2 p3 = ImVec2(c.x + m_EdgeVertices[triangle.v2].x, c.y + m_EdgeVertices[triangle.v2].y);
                    pDrawList->AddTriangle(p1, p2, p3, triangleColor);
                }
            }

            if (m_ShowEdgeVertices)
            {
                const ImU32 vertexColor = ImGui::GetColorU32(IM_COL32(255, 0, 0, 255));
                const int vertexRadius = 1;
                for (auto& vertex : m_EdgeVertices)
                {
                    ImVec2 p1 = ImVec2(c.x + vertex.x - vertexRadius, c.y + vertex.y - vertexRadius);
                    ImVec2 p2 = ImVec2(c.x + vertex.x + vertexRadius, c.y + vertex.y - vertexRadius);
                    ImVec2 p3 = ImVec2(c.x + vertex.x + vertexRadius, c.y + vertex.y + vertexRadius);
                    ImVec2 p4 = ImVec2(c.x + vertex.x - vertexRadius, c.y + vertex.y + vertexRadius);
                    pDrawList->AddQuadFilled(p1, p2, p3, p4, vertexColor);
                }
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

void Dome::CalculateEdgePoints()
{
    m_EdgePoints = m_pTextureProcessor->GetEdgePoints(m_pGreyscaleTexture.get(), m_EdgePointThreshold);
}

void Dome::BuildGeometry()
{
    m_EdgeVertices = m_pTextureProcessor->GetVertexFromPoints(m_EdgePoints, m_MaxVertexCount, m_Accuracy, m_pSourceTexture->GetWidth(), m_pSourceTexture->GetHeight());

    if (m_CollapseVertices)
    {
        m_EdgeVertices = m_pGeometryProcessor->GetUniqueVertices(m_EdgeVertices, m_CollapseThreshold);
    }

    m_IndexedTriangles = m_pGeometryProcessor->GetTriangles(m_EdgeVertices);

    m_ColorizedVertices = m_pTextureProcessor->GetColorizedVertices(m_EdgeVertices, m_pSourceTexture.get());
}

} // namespace WingsOfSteel::Dome