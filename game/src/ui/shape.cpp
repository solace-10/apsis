#include <sstream>

#include <imgui/imgui.hpp>

#include "ui/shape.hpp"

namespace WingsOfSteel::UI
{

Shape::Shape()
{
    AddFlag(Flags::AutoSize);
}

ElementType Shape::GetType() const
{
    return ElementType::Shape;
}

const std::string& Shape::GetIcon() const
{
    static const std::string icon(ICON_FA_DRAW_POLYGON);
    return icon;
}

void Shape::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cellSize = GetCellSize();

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    const std::vector<std::array<ImVec2, 3>> triangles = ParseTriangles();
    const ImVec2 domain(320, 288);
    for (const auto& tri : triangles)
    {
        const ImVec2 v0 = cp0 + ImVec2(tri[0].x * cellSize.x / domain.x, tri[0].y * cellSize.y / domain.y);
        const ImVec2 v1 = cp0 + ImVec2(tri[1].x * cellSize.x / domain.x, tri[1].y * cellSize.y / domain.y);
        const ImVec2 v2 = cp0 + ImVec2(tri[2].x * cellSize.x / domain.x, tri[2].y * cellSize.y / domain.y);
        pDrawList->AddTriangleFilled(v0, v1, v2, m_FillColor);
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        const ImVec2 cp1 = cp0 + cellSize;
        pDrawList->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void Shape::RenderProperties()
{
    StackableElement::RenderProperties();

    ImVec4 color = ImGui::ColorConvertU32ToFloat4(m_FillColor);
    if (ImGui::ColorEdit4("Fill Color", &color.x))
    {
        m_FillColor = ImGui::ColorConvertFloat4ToU32(color);
    }

    ImGui::InputTextMultiline("Triangles", &m_Triangles);
}

nlohmann::json Shape::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    data["triangles"] = m_Triangles;
    data["fill_color"] = {
        static_cast<int>((m_FillColor >> IM_COL32_R_SHIFT) & 0xFF),
        static_cast<int>((m_FillColor >> IM_COL32_G_SHIFT) & 0xFF),
        static_cast<int>((m_FillColor >> IM_COL32_B_SHIFT) & 0xFF),
        static_cast<int>((m_FillColor >> IM_COL32_A_SHIFT) & 0xFF)
    };
    return data;
}

void Shape::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    TryDeserialize(data, "triangles", m_Triangles, std::string(""));

    if (data.contains("fill_color") && data["fill_color"].is_array() && data["fill_color"].size() == 4)
    {
        const auto& c = data["fill_color"];
        m_FillColor = IM_COL32(
            c[0].get<int>(),
            c[1].get<int>(),
            c[2].get<int>(),
            c[3].get<int>()
        );
    }
}

std::vector<std::array<ImVec2, 3>> Shape::ParseTriangles() const
{
    std::vector<std::array<ImVec2, 3>> result;
    std::istringstream stream(m_Triangles);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::istringstream lineStream(line);
        std::array<ImVec2, 3> triangle;
        char comma;
        bool valid = true;

        for (int i = 0; i < 3 && valid; ++i)
        {
            if (!(lineStream >> triangle[i].x))
            {
                valid = false;
                break;
            }
            if (!(lineStream >> comma) || comma != ',')
            {
                valid = false;
                break;
            }
            if (!(lineStream >> triangle[i].y))
            {
                valid = false;
                break;
            }
            if (i < 2)
            {
                if (!(lineStream >> comma) || comma != ',')
                {
                    valid = false;
                }
            }
        }

        if (valid)
        {
            result.push_back(triangle);
        }
    }

    return result;
}

} // namespace WingsOfSteel::UI
