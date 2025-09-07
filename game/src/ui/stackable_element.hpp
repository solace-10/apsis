#pragma once

#include <glm/vec2.hpp>

#include "ui/element.hpp"

namespace WingsOfSteel::UI
{

DECLARE_SMART_PTR(Stack);

DECLARE_SMART_PTR(StackableElement);
class StackableElement : public Element
{
public:
    StackableElement();
    ~StackableElement() override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void SetStack(StackSharedPtr pStack);
    int GetCell() const;
    void SetCell(int cell);

protected:
    glm::vec2 GetCellPosition() const;
    glm::vec2 GetCellSize() const;

private:
    StackWeakPtr m_pStack;
    int m_Cell{0};
};

inline int StackableElement::GetCell() const
{
    return m_Cell;
}

inline void StackableElement::SetCell(int cell)
{
    m_Cell = cell;
}

} // namespace WingsOfSteel::UI