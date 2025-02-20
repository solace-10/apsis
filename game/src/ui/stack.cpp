#include <imgui/imgui.hpp>
#include <magic_enum.hpp>

#include "ui/stack.hpp"
#include "ui/ui.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Stack::Stack()
{

}

Stack::~Stack()
{

}

ElementType Stack::GetType() const
{
    return ElementType::Stack;
}

const std::string& Stack::GetIcon() const
{
    static const std::string icon(ICON_FA_BARS);
    return icon;
}

nlohmann::json Stack::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    data["orientation"] = magic_enum::enum_name(m_Orientation);
    data["cells"] = m_CellDefinition;

    if (!m_Elements.empty())
    {
        nlohmann::json elements;
        for (const auto& pElement : m_Elements)
        {
            elements.push_back(pElement->Serialize());
        }
        data["elements"] = elements;
    }

    return data;
}

void Stack::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    static const std::string orientationKey("orientation");
    if (data.contains(orientationKey))
    {
        const std::string& orientation = data[orientationKey];
        if (orientation == "horizontal")
        {
            m_Orientation = Orientation::Horizontal;
        }
        else if (orientation == "vertical")
        {
            m_Orientation = Orientation::Vertical;
        }
    }

    TryDeserialize(data, "cells", m_CellDefinition, "*");

    static const std::string elementsKey("elements");
    if (data.contains(elementsKey))
    {
        const nlohmann::json& elements = data[elementsKey];
        if (elements.is_array())
        {
            for (const auto& element : elements)
            {
                std::string type;
                if (TryDeserialize(element, "type", type, "null"))
                {
                    StackableElementSharedPtr pStackableElement = static_pointer_cast<StackableElement>(CreateElement(type));
                    if (pStackableElement)
                    {
                        AddElement(pStackableElement);
                        pStackableElement->Deserialize(element);

                        StackSharedPtr pStack = static_pointer_cast<Stack>(shared_from_this());
                        pStackableElement->SetStack(pStack, pStackableElement->GetCell());
                    }
                }
            }
        }
    }
}

void Stack::Render()
{
    for (const auto& pElement : m_Elements)
    {
        pElement->Render();
    }
}

void Stack::RenderProperties()
{
    StackableElement::RenderProperties();

    ImGui::InputText("Cells", &m_CellDefinition);
    //ImGui::Combo("Orientation", &m_Orientation, "Horizontal\0Vertical\0");
}

void Stack::AddElement(StackableElementSharedPtr pStackableElement)
{
    m_Elements.push_back(pStackableElement);
    
}

const std::vector<StackableElementSharedPtr>& Stack::GetElements() const
{
    return m_Elements;
}

glm::vec2 Stack::GetCellPosition(int cell) const
{
    return glm::vec2(0, 0);
}

glm::vec2 Stack::GetCellSize(int cell) const
{
    return glm::vec2(128, 128);
}

} // namespace WingsOfSteel::TheBrightestStar::UI
