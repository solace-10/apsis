#pragma once

#include <glm/vec2.hpp>

#include "ui/element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
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
    void Deserialize(const nlohmann::json& data) override;

    void SetStack(StackSharedPtr pStack, int cell);
    int GetCell() const;

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

} // namespace WingsOfSteel::TheBrightestStar::UI