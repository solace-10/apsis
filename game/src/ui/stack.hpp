#pragma once

#include <vector>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Stack);
class Stack : public StackableElement
{
public:
    enum class Orientation
    {
        Horizontal,
        Vertical,
    };

    Stack();
    ~Stack() override;

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;

    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void AddElement(StackableElementSharedPtr pStackableElement);

    const std::vector<StackableElementSharedPtr>& GetElements() const;

    glm::vec2 GetCellPosition(int cell) const;
    glm::vec2 GetCellSize(int cell) const;

private:
    std::vector<StackableElementSharedPtr> m_Elements;
    Orientation m_Orientation{Orientation::Vertical};
    std::string m_CellDefinition{"*"};
};

} // namespace WingsOfSteel::TheBrightestStar::UI
