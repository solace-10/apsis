#include <glm/common.hpp>
#include <imgui/imgui.hpp>
#include <magic_enum.hpp>

#include "ui/stack.hpp"
#include "ui/ui.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Stack::Stack()
{
    // Stacks always have auto size, as they're the base layout element.
    AddFlag(Flags::AutoSize);
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

void Stack::SetSize(const glm::ivec2& size)
{
    if (GetSize() != size)
    {
        StackableElement::SetSize(size);
        m_CellsDirty = true;
    }
}

void Stack::SetPosition(const glm::ivec2& position)
{
    if (GetPosition() != position)
    {
        StackableElement::SetPosition(position);
        m_CellsDirty = true;
    }
}

nlohmann::json Stack::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();

    data["orientation"] = magic_enum::enum_name(m_Orientation);
    data["cells"] = m_CellDefinitionDescription;

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

    TryDeserialize<Orientation>(data, "orientation", m_Orientation, Orientation::Horizontal);
    TryDeserialize(data, "cells", m_CellDefinitionDescription, "*");
    ProcessCellDefinitionDescription();

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
                    }
                }
            }
        }
    }
}

void Stack::Render()
{
    ImGui::SetCursorPos(GetPosition());

    const ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();
    if (cursorScreenPosition != m_CursorScreenPosition)
    {
        m_CellsDirty = true;
    }

    if (m_CellsDirty)
    {
        UpdateCells();
    }

    for (const auto& pElement : m_Elements)
    {
        ImGui::SetCursorScreenPos(cursorScreenPosition + GetCellPosition(pElement->GetCell()));
        pElement->Render();
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        for (const auto& cell : m_Cells)
        {
            if (m_Orientation == Orientation::Horizontal)
            {
                pDrawList->AddRect(cursorScreenPosition + ImVec2(cell.offset, 0), cursorScreenPosition + ImVec2(cell.offset + cell.size.x, cell.size.y), IM_COL32(255, 0, 0, 255));
            }
            else if (m_Orientation == Orientation::Vertical)
            {
                pDrawList->AddRect(cursorScreenPosition + ImVec2(0, cell.offset), cursorScreenPosition + ImVec2(cell.size.x, cell.offset + cell.size.y), IM_COL32(255, 0, 0, 255));
            }
        }
    }
}

void Stack::RenderProperties()
{
    StackableElement::RenderProperties();

    int orientation = static_cast<int>(m_Orientation);
    if (ImGui::Combo("Orientation", &orientation, "Horizontal\0Vertical\0"))
    {
        m_Orientation = static_cast<Orientation>(orientation);
    }

    if (ImGui::InputText("Cells", &m_CellDefinitionDescription))
    {
        ProcessCellDefinitionDescription();
    }

    if (!m_ValidCellDefinition)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid cell definition");
    }
}

void Stack::AddElement(StackableElementSharedPtr pStackableElement)
{
    m_Elements.push_back(pStackableElement);
    StackSharedPtr pStack = static_pointer_cast<Stack>(shared_from_this());
    pStackableElement->SetStack(pStack);
}

const std::vector<StackableElementSharedPtr>& Stack::GetElements() const
{
    return m_Elements;
}

glm::vec2 Stack::GetCellPosition(int cell) const
{
    if (cell >= 0 && cell < m_Cells.size())
    {
        if (m_Orientation == Orientation::Horizontal)
        {
            return glm::vec2(m_Cells[cell].offset, 0);
        }
        else if (m_Orientation == Orientation::Vertical)
        {
            return glm::vec2(0, m_Cells[cell].offset);
        }
    }

    return glm::vec2(0, 0);
}

glm::vec2 Stack::GetCellSize(int cell) const
{
    if (cell >= 0 && cell < m_Cells.size())
    {
        return m_Cells[cell].size;
    }

    return glm::vec2(0, 0);
}

void Stack::ProcessCellDefinitionDescription()
{
    m_CellDefinitions.clear();

    std::stringstream ss(m_CellDefinitionDescription);
    std::string cell;
    m_ValidCellDefinition = false;
    while (std::getline(ss, cell, ';'))
    {
        std::optional<CellDefinition> cellDefinition = ParseCellDefinition(cell);
        if (cellDefinition)
        {
            m_CellDefinitions.push_back(cellDefinition.value());
            m_ValidCellDefinition = true;
        }
        else
        {
            m_ValidCellDefinition = false;
            break;
        }
    }

    if (!m_ValidCellDefinition)
    {
        m_CellDefinitions.clear();
        m_CellDefinitions.push_back({CellDimensionType::Percentage, 100});
    }

    m_CellsDirty = true;
}

std::optional<Stack::CellDefinition> Stack::ParseCellDefinition(const std::string& cellDefinition) const
{
    if (cellDefinition == "*")
    {
        return CellDefinition(CellDimensionType::Auto);
    }
    else
    {
        try
        {
            const bool isPercentage = cellDefinition.find("%") == cellDefinition.size() - 1;
            if (isPercentage)
            {
                const int percentage = glm::clamp(std::stoi(cellDefinition.substr(0, cellDefinition.size() - 1)), 0, 100);
                return CellDefinition(CellDimensionType::Percentage, percentage);
            }
            else
            {
                return CellDefinition(CellDimensionType::Fixed, std::stoi(cellDefinition));
            }
        }
        catch (const std::exception& e)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

// Must only be called from Render(), otherwise there is no guarantee that ImGui::GetCursorScreenPos() will return the correct position.
void Stack::UpdateCells()
{
    m_Cells.clear();
    ImVec2 stackPosition = ImGui::GetCursorScreenPos();
    ImVec2 stackSize = GetSize();
    int stackLength = (m_Orientation == Orientation::Horizontal ? stackSize.x : stackSize.y);
    int remainingSpace = stackLength;
    int cellOffset = 0;
    
    int autoCellIndex = -1;
    const int numCellDefinitions = static_cast<int>(m_CellDefinitions.size());
    for (int i = 0; i < numCellDefinitions; ++i)
    {
        if (m_CellDefinitions[i].dimension == CellDimensionType::Auto)
        {
            autoCellIndex = i;
            break;
        }
    }

    for (int i = 0; i < numCellDefinitions; ++i)
    {
        CellDefinition& cellDefinition = m_CellDefinitions[i];
        if (cellDefinition.dimension == CellDimensionType::Fixed)
        {
            if (m_Orientation == Orientation::Horizontal)
            {
                m_Cells.emplace_back(cellOffset, glm::ivec2(cellDefinition.value, stackSize.y));
            }
            else if (m_Orientation == Orientation::Vertical)
            {
                m_Cells.emplace_back(cellOffset, glm::ivec2(stackSize.x, cellDefinition.value));
            }
            cellOffset += cellDefinition.value;
            remainingSpace -= cellDefinition.value;
        }
        else if (cellDefinition.dimension == CellDimensionType::Percentage)
        {
            int value = static_cast<int>(static_cast<float>(remainingSpace) * static_cast<float>(cellDefinition.value) / 100.0f);
            if (m_Orientation == Orientation::Horizontal)
            {
                m_Cells.emplace_back(cellOffset, glm::ivec2(value, stackSize.y));
            }
            else if (m_Orientation == Orientation::Vertical)
            {
                m_Cells.emplace_back(cellOffset, glm::ivec2(stackSize.x, value));
            }
            cellOffset += value;
            remainingSpace -= value;
        }
        else if (cellDefinition.dimension == CellDimensionType::Auto)
        {
            if (m_Orientation == Orientation::Horizontal)
            {
                m_Cells.emplace_back(cellOffset, glm::ivec2(0, stackSize.y));
            }
            else if (m_Orientation == Orientation::Vertical)
            {
                m_Cells.emplace_back(cellOffset, glm::ivec2(stackSize.x, 0));
            }
        }
    }

    if (autoCellIndex != -1)
    {
        if (m_Orientation == Orientation::Horizontal)
        {
            m_Cells[autoCellIndex].size.x = remainingSpace;
        }
        else if (m_Orientation == Orientation::Vertical)
        {
            m_Cells[autoCellIndex].size.y = remainingSpace;
        }

        for (int i = autoCellIndex + 1; i < numCellDefinitions; ++i)
        {
            m_Cells[i].offset += remainingSpace;
        }
    }

    m_CellsDirty = false;
}

} // namespace WingsOfSteel::TheBrightestStar::UI
