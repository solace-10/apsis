#include <imgui/imgui.hpp>

#include "ui/stackable_element.hpp"
#include "ui/stack.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

StackableElement::StackableElement()
{
}

StackableElement::~StackableElement()
{
}

void StackableElement::Render()
{
}

void StackableElement::RenderProperties()
{
    StackSharedPtr pStack = m_pStack.lock();
    if (pStack)
    {
        ImGui::InputInt("Stack cell", &m_Cell);
    }
}

void StackableElement::Deserialize(const nlohmann::json& data)
{
    Element::Deserialize(data);
    TryDeserialize(data, "cell", m_Cell, 0);
}

void StackableElement::SetStack(StackSharedPtr pStack, int cell)
{
    m_pStack = pStack;
    m_Cell = cell;
}

glm::vec2 StackableElement::GetCellPosition() const
{
    StackSharedPtr pStack = m_pStack.lock();
    if (pStack)
    {
        return pStack->GetCellPosition(m_Cell);
    }
    else
    {
        return glm::vec2(0, 0);
    }
}

glm::vec2 StackableElement::GetCellSize() const
{
    StackSharedPtr pStack = m_pStack.lock();
    if (pStack)
    {
        return pStack->GetCellSize(m_Cell);
    }
    else
    {
        return glm::vec2(0, 0);
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI