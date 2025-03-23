#include <glm/glm.hpp>

#include <core/log.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <vfs/file.hpp>
#include <vfs/vfs.hpp>
#include <pandora.hpp>

#include "encounter_editor.hpp"

#include "encounter_blueprint_nodes.hpp"
#include "encounter_editor_widgets.hpp"
#include "encounter.hpp"

namespace WingsOfSteel::TheBrightestStar
{

EncounterEditor::EncounterEditor()
{
    m_pContext = ImGuiNodeEditor::CreateEditor();

    using namespace Pandora;
    const std::vector<std::string> encounters = GetVFS()->List("/encounters");
    bool firstEncounterRequested = false;
    for (const std::string& encounter : encounters)
    {
        const std::string encounterName = GetEncounterName(encounter);
        m_Encounters[encounterName] = nullptr;

        if (!firstEncounterRequested)
        {
            firstEncounterRequested = true;
            LoadEncounter(encounterName);
        }
    }
}

EncounterEditor::~EncounterEditor()
{
    ImGuiNodeEditor::DestroyEditor(m_pContext);
}

void EncounterEditor::Update()
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Encounter Editor");
    DrawHeader();
    DrawEncounterList();
    ImGui::SameLine();
    DrawNodeEditor();
    ImGui::End();

    DrawStringEditor();
}

void EncounterEditor::DrawHeader()
{
    if (ImGui::Button(ICON_FA_FILE_CIRCLE_PLUS " New"))
    {
        m_NewEncounterName = "";
        ImGui::OpenPopup("New encounter");
    }
    ImGui::SameLine();

    bool fileLoaded = true;
    ImGui::BeginDisabled(!fileLoaded);
    if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
    {
        SaveEncounter();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ROTATE_LEFT " Revert"))
    {
        RevertEncounter();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_GEAR " Settings"))
    {
        //ShowSettings();
    }
    ImGui::EndDisabled();

    if (ImGui::BeginPopupModal("New encounter", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::InputText("##encounterName", &m_NewEncounterName);
        if (ImGui::Button("Create"))
        {
            AddNewEmptyEncounter();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EncounterEditor::DrawEncounterList()
{
    ImGui::BeginChild("EncounterList", ImVec2(300, 0), ImGuiChildFlags_Border);
    for (const auto& encounter : m_Encounters)
    {
        const std::string label = ICON_FA_FILE_CODE " " + encounter.first;
        if (ImGui::Selectable(label.c_str(), m_pSelectedEncounter != nullptr && m_pSelectedEncounter == encounter.second))
        {
            if (encounter.second == nullptr)
            {
                LoadEncounter(encounter.first);
            }
            else
            {
                m_pSelectedEncounter = encounter.second;
            }
        }
    }
    ImGui::EndChild();
}

void EncounterEditor::DrawNodeEditor()
{
    ImGuiNodeEditor::SetCurrentEditor(m_pContext);
    ImGuiNodeEditor::Begin("Encounter Editor");
    DrawNodes();
    DrawContextMenus();
    ImGuiNodeEditor::End();
    ImGuiNodeEditor::SetCurrentEditor(nullptr);
}

void EncounterEditor::DrawStringEditor()
{
    if (m_ShowStringEditor)
    {
        ImGui::OpenPopup("String editor");
    }

    if (ImGui::BeginPopupModal("String editor", &m_ShowStringEditor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::InputTextMultiline("##hidden", &m_pSelectedStringNode->Value, ImVec2(600, ImGui::GetTextLineHeightWithSpacing() * 16));
        ImGui::EndPopup();
    }
}
void EncounterEditor::DrawNodes()
{
    if (m_pSelectedEncounter == nullptr)
    {
        return;
    }

    const int nodeTitleHeight = 36;
    for (Node* pNode : m_pSelectedEncounter->GetNodes())
    {
        ImGuiNodeEditor::BeginNode(pNode->ID);

        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImVec2 textSize = ImGui::CalcTextSize(pNode->Name.c_str());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
        ImGui::Dummy(ImVec2(textSize.x, nodeTitleHeight));
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
        if (pNode->Type == NodeType::Standard)
        {
            DrawStandardNode(pNode);
        }
        else if (pNode->Type == NodeType::String)
        {
            DrawStringNode(pNode);
        }
        ImGui::PopStyleVar();

        ImGui::SetCursorPos(cursorPos);
        
        ImVec2 nodeSize = ImGuiNodeEditor::GetNodeSize(pNode->ID);
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        ImVec2 nodePadding = ImVec2(8, 8);
        ImVec2 cp0 = ImGui::GetCursorScreenPos() - nodePadding + ImVec2(1, 1);
        ImVec2 cp1 = cp0 + ImVec2(nodeSize.x, nodeTitleHeight) - ImVec2(2, 2);
        ImVec4 titleColor(pNode->Color);
        titleColor.w = 0.7f; // Make the title color a bit darker by reducing the alpha value.
        pDrawList->AddRectFilled(cp0, cp1, ImColor(titleColor), 12.0, ImDrawFlags_RoundCornersTop);

        pDrawList->AddText(cp0 + ImVec2(8, 4), IM_COL32(255, 255, 255, 255), pNode->Name.c_str());

        ImGuiNodeEditor::EndNode();
    }
}

void EncounterEditor::DrawStandardNode(Node* pNode)
{
    DrawPins(pNode);
}

void EncounterEditor::DrawStringNode(Node* pNode)
{
    const int buttonWidth = 40;
    ImVec2 pinsStartPos = ImGui::GetCursorPos();

    if (ImGui::Button(ICON_FA_PEN, ImVec2(buttonWidth, 0)))
    {
        m_ShowStringEditor = true;
        m_pSelectedStringNode = static_cast<StringNode*>(pNode);
    }

    const int outputPinsWidth = GetPinGroupWidth(pNode->Outputs);
    const int contentWidth = outputPinsWidth + buttonWidth + 8;
    const int titleWidth = ImGui::CalcTextSize(pNode->Name.c_str()).x + 16;
    const ImVec2 outputPinsStartPos = pinsStartPos + ImVec2(glm::max(contentWidth, titleWidth), 4);
    ImVec2 outputPinPos = pinsStartPos + ImVec2(glm::max(contentWidth, titleWidth), 4);;

    assert(pNode->Outputs.size() == 1);
    Pin& outputPin = pNode->Outputs[0];
    outputPinPos.x = outputPinsStartPos.x - GetPinWidth(outputPin);
    ImGui::SetCursorPos(outputPinPos);

    ImGuiNodeEditor::BeginPin(outputPin.ID, ImGuiNodeEditor::PinKind::Output);
    ImGui::TextUnformatted(outputPin.Name.c_str());
    ImGui::SameLine();
    DrawPinIcon(outputPin, false);
    ImGuiNodeEditor::EndPin();
}

int EncounterEditor::GetPinGroupWidth(const std::vector<Pin>& pins) const
{
    const int itemSpacing = static_cast<int>(ImGui::GetStyle().ItemSpacing.x);
    int maximumWidth = 0;
    for (const Pin& pin : pins)
    {   
        const int pinWidth = itemSpacing + GetPinIconSize() + ImGui::CalcTextSize(pin.Name.c_str()).x;
        if (pinWidth > maximumWidth)
        {
            maximumWidth = pinWidth;
        }
    }
    return maximumWidth;
}

void EncounterEditor::DrawPins(Node* pNode)
{
    const int spaceBetweenPins = 28; 
    ImVec2 pinsStartPos = ImGui::GetCursorPos();
    ImVec2 inputPinPos = pinsStartPos;
    for (Pin& inputPin : pNode->Inputs)
    {
        ImGuiNodeEditor::BeginPin(inputPin.ID, ImGuiNodeEditor::PinKind::Input);
        DrawPinIcon(inputPin, false);
        ImGui::SameLine();
        ImGui::TextUnformatted(inputPin.Name.c_str());
        ImGuiNodeEditor::EndPin();
        inputPinPos.y += spaceBetweenPins;
        ImGui::SetCursorPos(inputPinPos);
    }
    const int inputPinsWidth = GetPinGroupWidth(pNode->Inputs);
    const int outputPinsWidth = GetPinGroupWidth(pNode->Outputs);
    const int combinedPinsWidth = inputPinsWidth + outputPinsWidth + 16; // + 16 to create a gap between the two pin groups.
    const int titleWidth = ImGui::CalcTextSize(pNode->Name.c_str()).x + 16;
    const ImVec2 outputPinsStartPos = pinsStartPos + ImVec2(glm::max(combinedPinsWidth, titleWidth), 0);
    ImVec2 outputPinPos = outputPinsStartPos;

    for (Pin& outputPin : pNode->Outputs)
    {
        outputPinPos.x = outputPinsStartPos.x - GetPinWidth(outputPin);
        ImGui::SetCursorPos(outputPinPos);

        ImGuiNodeEditor::BeginPin(outputPin.ID, ImGuiNodeEditor::PinKind::Output);
        ImGui::TextUnformatted(outputPin.Name.c_str());
        ImGui::SameLine();
        DrawPinIcon(outputPin, false);
        ImGuiNodeEditor::EndPin();
        outputPinPos.y += spaceBetweenPins;
        ImGui::SetCursorPos(outputPinPos);
    }
}

void EncounterEditor::DrawPinIcon(const Pin& pin, bool connected)
{
    PinIconType iconType;
    ImColor color = GetIconColor(pin.Type);
    switch (pin.Type)
    {
        case PinType::Flow: iconType = PinIconType::Flow; break;
        case PinType::Dice: iconType = PinIconType::Square; break;
        case PinType::String: iconType = PinIconType::RoundSquare; break;
        default: iconType = PinIconType::Circle; break;
    }

    const int pinIconSize = GetPinIconSize(); 
    PinIcon(ImVec2(pinIconSize, pinIconSize), iconType, connected, color, ImColor(32, 32, 32, 255));
}

ImColor EncounterEditor::GetIconColor(PinType type) const
{
    switch (type)
    {
        case PinType::Flow: return ImColor(255, 255, 255);
        case PinType::Dice: return ImColor(255, 0, 0);
        case PinType::String: return ImColor(124, 21, 153);
        default: return ImColor(255, 255, 255);
    }
}

void EncounterEditor::AddNewEmptyEncounter()
{
    using namespace Pandora;
    nlohmann::json json = nlohmann::json::object();
    const std::string dataDump = json.dump(4);
    std::vector<uint8_t> data(dataDump.begin(), dataDump.end());
    const std::string path = "/encounters/" + m_NewEncounterName + ".json";
    if (GetVFS()->FileWrite(path, data))
    {
        Log::Info() << "Created new empty encounter at '" << path << "'.";
        LoadEncounter(m_NewEncounterName);
        m_NewEncounterName = "";
    }
    else
    {
        Log::Warning() << "Failed to create new empty encounter at '" << path << "'.";
    }
}

void EncounterEditor::LoadEncounter(const std::string& encounterName)
{
    using namespace Pandora;
    const std::string path = "/encounters/" + encounterName + ".json";
    GetResourceSystem()->RequestResource(path, [this, encounterName](ResourceSharedPtr pResource)
    {
        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        if (pDataStore)
        {
            m_pSelectedEncounter = std::make_shared<Encounter>(pDataStore);
            m_Encounters[encounterName] = m_pSelectedEncounter;
        }
    });
}

void EncounterEditor::SaveEncounter()
{
    if (m_pSelectedEncounter)
    {
        m_pSelectedEncounter->Save();
    }
}

void EncounterEditor::RevertEncounter()
{
    if (m_pSelectedEncounter)
    {
        m_pSelectedEncounter->Revert();
    }
}

std::string EncounterEditor::GetEncounterName(const std::string& path) const
{
    const size_t lastSlashIndex = path.find_last_of("/");
    if (lastSlashIndex == std::string::npos)
    {
        return path;
    }
    const size_t lastDotIndex = path.find_last_of(".");
    if (lastDotIndex == std::string::npos)
    {
        return path;
    }
    return path.substr(lastSlashIndex + 1, lastDotIndex - lastSlashIndex - 1);
}

void EncounterEditor::DrawContextMenus()
{
    if (m_pSelectedEncounter == nullptr)
    {
        return;
    }

    auto openPopupPosition = ImGui::GetMousePos();
    ImGuiNodeEditor::Suspend();
    //if (ImGuiNodeEditor::ShowNodeContextMenu(&contextNodeId))
    //    ImGui::OpenPopup("Node Context Menu");
    //else if (ImGuiNodeEditor::ShowPinContextMenu(&contextPinId))
    //    ImGui::OpenPopup("Pin Context Menu");
    //else if (ImGuiNodeEditor::ShowLinkContextMenu(&contextLinkId))
    //    ImGui::OpenPopup("Link Context Menu");

    if (ImGuiNodeEditor::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("Create New Node");
    }
    ImGuiNodeEditor::Resume();

    ImGuiNodeEditor::Suspend();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    /*
    if (ImGui::BeginPopup("Node Context Menu"))
    {
        auto node = FindNode(contextNodeId);

        ImGui::TextUnformatted("Node Context Menu");
        ImGui::Separator();
        if (node)
        {
            ImGui::Text("ID: %p", node->ID.AsPointer());
            ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
            ImGui::Text("Inputs: %d", (int)node->Inputs.size());
            ImGui::Text("Outputs: %d", (int)node->Outputs.size());
        }
        else
            ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
            ImGuiNodeEditor::DeleteNode(contextNodeId);
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Pin Context Menu"))
    {
        auto pin = FindPin(contextPinId);

        ImGui::TextUnformatted("Pin Context Menu");
        ImGui::Separator();
        if (pin)
        {
            ImGui::Text("ID: %p", pin->ID.AsPointer());
            if (pin->Node)
                ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
            else
                ImGui::Text("Node: %s", "<none>");
        }
        else
            ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Link Context Menu"))
    {
        auto link = FindLink(contextLinkId);

        ImGui::TextUnformatted("Link Context Menu");
        ImGui::Separator();
        if (link)
        {
            ImGui::Text("ID: %p", link->ID.AsPointer());
            ImGui::Text("From: %p", link->StartPinID.AsPointer());
            ImGui::Text("To: %p", link->EndPinID.AsPointer());
        }
        else
            ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
            ImGuiNodeEditor::DeleteLink(contextLinkId);
        ImGui::EndPopup();
    }
    */

    if (ImGui::BeginPopup("Create New Node"))
    {
        auto newNodePostion = openPopupPosition;

        NodeUniquePtr pNode = nullptr;
        if (ImGui::MenuItem("Sector entered"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Sector entered");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Encounter stage"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Encounter stage");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("String"))
        {
            pNode = BlueprintNodeFactory::CreateNode("String");
        }

        if (pNode)
        {
            pNode->Initialize(m_IdGenerator);
            ImGuiNodeEditor::SetNodePosition(pNode->ID, newNodePostion);
            m_pSelectedEncounter->AddNode(std::move(pNode));
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
    ImGuiNodeEditor::Resume();
}

} // namespace WingsOfSteel::TheBrightestStar
