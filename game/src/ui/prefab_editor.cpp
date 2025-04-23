#include <sstream>

#include <imgui/imgui.hpp>

#include "ui/prefab_editor.hpp"
#include "ui/window.hpp"
#include "ui/stack.hpp"
#include "ui/theme.hpp"
#include "ui/ui.hpp"
#include "game.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

PrefabEditor::PrefabEditor()
{

}

PrefabEditor::~PrefabEditor()
{

}

void PrefabEditor::Initialize()
{

}

void PrefabEditor::AddPrefabData(const std::string& prefabPath, WindowSharedPtr pWindow)
{
    m_RegisteredPrefabs.push_back({prefabPath, pWindow});
}

void PrefabEditor::ShowPrefabEditor(bool state)
{
    m_ShowPrefabEditor = state;
}

void PrefabEditor::DrawPrefabEditor()
{
    if (!m_ShowPrefabEditor)
    {
        return;
    }

    ImGui::Begin("Prefab Editor", &m_ShowPrefabEditor);

    bool prefabLoaded = m_pSelectedPrefab.has_value();
    ImGui::BeginDisabled(!prefabLoaded);

    if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
    {
        Save();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ROTATE_LEFT " Revert"))
    {
        Revert();
    }
    ImGui::EndDisabled();

    ImGui::BeginChild("PrefabList", ImVec2(300, 0), ImGuiChildFlags_Border);
    for (const auto& registeredPrefab : m_RegisteredPrefabs)
    {
        WindowSharedPtr pWindow = registeredPrefab.pWindow.lock();
        if (pWindow)
        {
            const std::string label = ICON_FA_FILE_CODE " " + registeredPrefab.prefabPath;
            if (ImGui::Selectable(label.c_str(), m_pSelectedPrefab.has_value() && m_pSelectedPrefab->prefabPath == registeredPrefab.prefabPath))
            {
                m_pSelectedPrefab = registeredPrefab;
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    RenderHierarchy();
    ImGui::SameLine();
    RenderProperties();
    ImGui::End();
}

void PrefabEditor::RenderHierarchy()
{
    ImGui::BeginChild("Hierarchy", ImVec2(500, 0), ImGuiChildFlags_Border);

    if (m_pSelectedPrefab.has_value())
    {
        WindowSharedPtr pWindow = m_pSelectedPrefab->pWindow.lock();
        if (pWindow)
        {
            RenderTreeElement(pWindow);
            HandleElementPopup();
        }
    }
    else
    {
        ImGui::TextDisabled("No prefab selected");
    }

    ImGui::EndChild();
}

void PrefabEditor::RenderTreeElement(ElementSharedPtr pElement)
{
    const bool isBound = pElement->HasFlag(Element::Flags::Bound);
    std::stringstream label;
    label << pElement->GetIcon() << " ";
    if (isBound)
    {
        label << ICON_FA_LINK << " ";
    }
    label <<  pElement->GetName();

    bool isLeaf = true;
    if (pElement->GetType() == ElementType::Window && static_pointer_cast<Window>(pElement)->GetStack())
    {
        isLeaf = false;
    }
    else if (pElement->GetType() == ElementType::Stack && static_pointer_cast<Stack>(pElement)->GetElements().size() > 0)
    {
        isLeaf = false;
    }

    ImGuiTreeNodeFlags nodeFlags = 0;
    if (m_pSelectedElement.lock() == pElement)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    if (isBound)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)Theme::AccentColor);
    }

    if (isLeaf)
    {
        ImGui::TreeNodeEx(label.str().c_str(), nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        HandleClickedEvent(pElement);
    }
    else if (ImGui::TreeNodeEx(label.str().c_str(), nodeFlags | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick))
    {
        HandleClickedEvent(pElement);

        if (pElement->GetType() == ElementType::Window)
        {
            StackSharedPtr pStack = static_pointer_cast<Window>(pElement)->GetStack();
            if (pStack)
            {
                RenderTreeElement(pStack);
            }
        }
        else if (pElement->GetType() == ElementType::Stack)
        {
            for (const auto& pElement : static_pointer_cast<Stack>(pElement)->GetElements())
            {
                RenderTreeElement(pElement);
            }
        }

        ImGui::TreePop();
    }
    
    if (isBound)
    {
        ImGui::PopStyleColor();
    }
}

void PrefabEditor::HandleClickedEvent(ElementSharedPtr pElement)
{
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        SelectElement(pElement);
    }
    else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        SelectElement(pElement);
        m_ElementPopup = true;
    }
}

void PrefabEditor::HandleElementPopup()
{
    ElementSharedPtr pSelectedElement = m_pSelectedElement.lock();
    if (!pSelectedElement)
    {
        return;
    }

    if (m_ElementPopup)
    {
        ImGui::OpenPopup("element_popup");
        m_ElementPopup = false;
    }

    if (ImGui::BeginPopup("element_popup"))
    {
        if (ImGui::MenuItem(ICON_FA_ROAD " Copy path"))
        {
            CopyPath(pSelectedElement.get());
        }

        if (pSelectedElement->GetType() == ElementType::Stack)
        {
            StackSharedPtr pStack = std::static_pointer_cast<Stack>(pSelectedElement);
            if (pStack)
            {
                ImGui::Separator();

                ElementSharedPtr pNewElement;
                if (ImGui::MenuItem(ICON_FA_CODE_BRANCH " Button"))
                {
                    pNewElement = CreateElement("Button");
                }
                if (ImGui::MenuItem(ICON_FA_DICE_FIVE " Dice"))
                {
                    pNewElement = CreateElement("Dice");
                }
                if (ImGui::MenuItem(ICON_FA_DIVIDE " Divider"))
                {
                    pNewElement = CreateElement("Divider");
                }
                if (ImGui::MenuItem(ICON_FA_HEADING " Heading"))
                {
                    pNewElement = CreateElement("Heading");
                }
                if (ImGui::MenuItem(ICON_FA_IMAGE " Image"))
                {
                    pNewElement = CreateElement("Image");
                }
                if (ImGui::MenuItem(ICON_FA_BARS " Stack"))
                {
                    pNewElement = CreateElement("Stack");
                }
                if (ImGui::MenuItem(ICON_FA_FONT " Text"))
                {
                    pNewElement = CreateElement("Text");
                }

                if (pNewElement)
                {
                    StackableElementSharedPtr pStackableElement = std::static_pointer_cast<StackableElement>(pNewElement);
                    const std::string defaultName = std::string("Default").append(magic_enum::enum_name(pNewElement->GetType()));
                    pNewElement->SetName(defaultName);
                    pStack->AddElement(pStackableElement);
                }
            }
        }

        ImGui::EndPopup();
    }
}

void PrefabEditor::CopyPath(Element* pElement)
{
    std::vector<Element*> hierarchy;
    hierarchy.push_back(pElement);
    for (Element* pParentElement = pElement->GetParent(); pParentElement != nullptr; pParentElement = pParentElement->GetParent())
    {
        hierarchy.push_back(pParentElement);
    }

    std::string path("/");
    for (int i = hierarchy.size() - 1; i >= 0; i--)
    {
        path += hierarchy[i]->GetName();
        if (i > 0)
        {
            path += "/";
        }
    }

    ImGui::SetClipboardText(path.c_str());
}

void PrefabEditor::RenderProperties()
{
    ImGui::BeginChild("Properties", ImVec2(0, 0), ImGuiChildFlags_Border);
    ElementSharedPtr pSelectedElement = m_pSelectedElement.lock();
    if (pSelectedElement)
    {
        pSelectedElement->RenderProperties();
    }
    else
    {
        ImGui::TextDisabled("No element selected");
    }
    ImGui::EndChild();
}

void PrefabEditor::Save()
{
    if (!m_pSelectedPrefab.has_value())
    {
        return;
    }

    WindowSharedPtr pWindow = m_pSelectedPrefab->pWindow.lock();
    if (!pWindow)
    {
        return;
    }

    nlohmann::json data = pWindow->Serialize();
    pWindow->GetDataStore()->Inject(data);
}

void PrefabEditor::Revert()
{

}

void PrefabEditor::SelectElement(ElementSharedPtr pElement)
{
    ElementSharedPtr pPreviousSelectedElement = m_pSelectedElement.lock();
    if (pPreviousSelectedElement)
    {
        pPreviousSelectedElement->RemoveFlag(Element::Flags::SelectedInEditor);
    }

    m_pSelectedElement = pElement;
    pElement->AddFlag(Element::Flags::SelectedInEditor);
}

} // namespace WingsOfSteel::TheBrightestStar::UI