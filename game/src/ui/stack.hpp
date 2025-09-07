#pragma once

#include <optional>
#include <vector>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::UI
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
    void SetSize(const glm::ivec2& size) override;
    void SetPosition(const glm::ivec2& position) override;

    void Render() override;
    void RenderProperties() override;

    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void AddElement(StackableElementSharedPtr pStackableElement);

    const std::vector<StackableElementSharedPtr>& GetElements() const;

    glm::vec2 GetCellPosition(int cell) const;
    glm::vec2 GetCellSize(int cell) const;

private:
    enum class CellDimensionType
    {
        Fixed,
        Percentage,
        Auto
    };

    struct CellDefinition
    {
        CellDefinition()
        {
            dimension = CellDimensionType::Auto;
            value = 0;
        }

        CellDefinition(CellDimensionType dimension, int value = 0)
        {
            this->dimension = dimension;
            this->value = value;
        }

        CellDimensionType dimension;
        int value;
    };

    struct Cell
    {
        Cell()
        {
            offset = 0;
            size = glm::ivec2(0);
        }

        Cell(int offset, glm::ivec2 size)
        {
            this->offset = offset;
            this->size = size;
        }

        int offset;
        glm::ivec2 size;
    };
    
    void ProcessCellDefinitionDescription();
    std::optional<CellDefinition> ParseCellDefinition(const std::string& cellDefinition) const;
    void UpdateCells();

    std::vector<StackableElementSharedPtr> m_Elements;
    Orientation m_Orientation{Orientation::Vertical};
    std::string m_CellDefinitionDescription{"*"};
    std::vector<CellDefinition> m_CellDefinitions;
    bool m_ValidCellDefinition{true};
    bool m_CellsDirty{true};
    std::vector<Cell> m_Cells;
    glm::ivec2 m_CursorScreenPosition{0};
};

} // namespace WingsOfSteel::UI
