#pragma once

#include <vector>

#include <imgui/imgui.hpp>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::UI
{

DECLARE_SMART_PTR(Shape);
class Shape : public StackableElement
{
public:
    Shape();
    ~Shape() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

private:
    std::vector<std::array<ImVec2, 3>> ParseTriangles() const;

    std::string m_Triangles;
    ImU32 m_FillColor{IM_COL32(255, 255, 255, 255)};
};

} // namespace WingsOfSteel::UI
