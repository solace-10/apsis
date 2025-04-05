#include <sstream>

#include <glm/glm.hpp>

#include <core/log.hpp>
#include <imgui/text_editor/text_editor.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <vfs/file.hpp>
#include <vfs/vfs.hpp>
#include <pandora.hpp>

#include "sector/encounters/encounter_editor.hpp"
#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "sector/encounters/encounter_editor_widgets.hpp"
#include "sector/encounters/encounter.hpp"
#include "sector/sector.hpp"
#include "game.hpp"

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
    bool encounterPlaying = (m_pSelectedEncounter != nullptr && m_pSelectedEncounter->IsPlaying());
    ImGui::BeginDisabled(!fileLoaded || encounterPlaying);
    if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
    {
        // The save needs to be deferred until we are inside the ImGuiNodeEditor logic, as the node
        // positions are not available until then.
        m_SaveEnqueued = true;
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
                CreateIdGenerator();
                ResetEditor();
            }
        }
    }
    ImGui::EndChild();
}

void EncounterEditor::DrawNodeEditor()
{
    ImGuiNodeEditor::SetCurrentEditor(m_pContext);
    ImGuiNodeEditor::Begin("Encounter Editor");

    if (m_LoadEnqueued)
    {
        RepositionNodes();
        m_LoadEnqueued = false;
    }

    DrawNodes();
    DrawLinks();
    DrawContextMenus();
    UpdateEvents();

    if (m_SaveEnqueued)
    {
        SaveEncounter();
        m_SaveEnqueued = false;
    }

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
        if (m_pSelectedStringNode->Editor == nullptr)
        {
            m_pSelectedStringNode->Editor = std::make_unique<Pandora::TextEditor>();
            m_pSelectedStringNode->Editor->SetLanguageDefinition(Pandora::TextEditor::LanguageDefinition());
            m_pSelectedStringNode->Editor->SetText(m_pSelectedStringNode->Value);
        }

        m_pSelectedStringNode->Editor->Render("String editor", ImVec2(600, ImGui::GetTextLineHeightWithSpacing() * 16));
        m_pSelectedStringNode->Value = m_pSelectedStringNode->Editor->GetText();
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
        if (pNode->Type == NodeDisplayType::Standard)
        {
            DrawStandardNode(pNode);
        }
        else if (pNode->Type == NodeDisplayType::String)
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

void EncounterEditor::DrawLinks()
{
    if (m_pSelectedEncounter)
    {
        for (Link* pLink : m_pSelectedEncounter->GetLinks())
        {
            if (!pLink->Color.has_value())
            {
                Pin* pPin = m_pSelectedEncounter->GetPin(pLink->StartPinID.Get());
                if (pPin)
                {
                    pLink->Color = GetIconColor(pPin->Type);
                }
            }

            ImGuiNodeEditor::Link(pLink->ID, pLink->StartPinID, pLink->EndPinID, pLink->Color.value_or(ImColor(255, 255, 255)));
        }
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

    std::stringstream buttonName;
    buttonName << ICON_FA_PEN << "##" << pNode->ID.Get();
    if (ImGui::Button(buttonName.str().c_str(), ImVec2(buttonWidth, 0)))
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
        case PinType::Outcome: iconType = PinIconType::Square; break;
        case PinType::String: iconType = PinIconType::RoundSquare; break;
        case PinType::EncounterOption: iconType = PinIconType::Diamond; break;
        case PinType::Dice: iconType = PinIconType::Square; break;
        case PinType::Image: iconType = PinIconType::Grid; break;
        default: iconType = PinIconType::Circle; break;
    }

    const int pinIconSize = GetPinIconSize(); 
    const ImVec2 pinCenter = ImGui::GetCursorScreenPos() + ImVec2(pinIconSize / 2, pinIconSize / 2);
    ImGuiNodeEditor::PinPivotRect(pinCenter, pinCenter);

    PinIcon(ImVec2(pinIconSize, pinIconSize), iconType, pin.Connected, color, ImColor(32, 32, 32, 255));
}

ImColor EncounterEditor::GetIconColor(PinType type) const
{
    switch (type)
    {
        case PinType::Flow: return ImColor(255, 255, 255);
        case PinType::Outcome: return ImColor(255, 0, 0);
        case PinType::String: return ImColor(204, 61, 233);
        case PinType::EncounterOption: return ImColor(5, 250, 191);
        case PinType::Dice: return ImColor(255, 165, 0);
        case PinType::Image: return ImColor(165, 255, 0);
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
    Sector* pSector = Game::Get()->GetSector();
    if (pSector != nullptr)
    {
        EncounterSharedPtr pEncounter = pSector->GetEncounter();
        if (pEncounter != nullptr)
        {
            OnEncounterLoaded(pEncounter);
            return;
        }
    }

    using namespace Pandora;
    const std::string path = "/encounters/" + encounterName + ".json";
    GetResourceSystem()->RequestResource(path, [this, encounterName](ResourceSharedPtr pResource)
    {
        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        if (pDataStore)
        {
            OnEncounterLoaded(std::make_shared<Encounter>(encounterName, pDataStore));
        }
    });
}

void EncounterEditor::OnEncounterLoaded(EncounterSharedPtr pEncounter)
{
    m_pSelectedEncounter = pEncounter;
    m_Encounters[m_pSelectedEncounter->GetName()] = m_pSelectedEncounter;
    m_LoadEnqueued = true;
    CreateIdGenerator();
    ResetEditor();
}

void EncounterEditor::SaveEncounter()
{
    if (m_pSelectedEncounter && !m_pSelectedEncounter->IsPlaying())
    {
        // Update the node positions for serialization when we're saving; no need to do this earlier.
        for (Node* pNode : m_pSelectedEncounter->GetNodes())
        {
            pNode->Position = ImGuiNodeEditor::GetNodePosition(pNode->ID);
        }

        m_pSelectedEncounter->Save();
    }
}

void EncounterEditor::RevertEncounter()
{
    if (m_pSelectedEncounter && !m_pSelectedEncounter->IsPlaying())
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
    if (m_pSelectedEncounter == nullptr || m_pSelectedEncounter->IsPlaying())
    {
        return;
    }

    auto openPopupPosition = ImGui::GetMousePos();
    ImGuiNodeEditor::Suspend();
    //if (ImGuiNodeEditor::ShowNodeContextMenu(&contextNodeId))
    //    ImGui::OpenPopup("Node Context Menu");

    if (ImGuiNodeEditor::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("Create New Node");
    }
    ImGuiNodeEditor::Resume();

    ImGuiNodeEditor::Suspend();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    if (ImGui::BeginPopup("Create New Node"))
    {
        auto newNodePostion = openPopupPosition;

        NodeUniquePtr pNode = nullptr;
        if (ImGui::MenuItem("Sector entered"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Sector entered");
        }
        if (ImGui::MenuItem("Sector exit"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Sector exit");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Encounter option"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Encounter option");
        }
        if (ImGui::MenuItem("Encounter stage"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Encounter stage");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Dice"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Dice");
        }
        if (ImGui::MenuItem("Image"))
        {
            pNode = BlueprintNodeFactory::CreateNode("Image");
        }
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

void EncounterEditor::RepositionNodes()
{
    for (Node* pNode : m_pSelectedEncounter->GetNodes())
    {
        ImGuiNodeEditor::SetNodePosition(pNode->ID, pNode->Position);
    }
}

void EncounterEditor::CreateIdGenerator()
{
    if (m_pSelectedEncounter == nullptr)
    {
        m_IdGenerator = EncounterEditorIdGenerator();
    }
    else
    {
        BlueprintId highestId = 0;
        for (Node* pNode : m_pSelectedEncounter->GetNodes())
        {
            if (pNode->ID.Get() > highestId)
            {
                highestId = pNode->ID.Get();
            }

            for (Pin& inputPin : pNode->Inputs)
            {
                if (inputPin.ID.Get() > highestId)
                {
                    highestId = inputPin.ID.Get();
                }
            }

            for (Pin& outputPin : pNode->Outputs)
            {
                if (outputPin.ID.Get() > highestId)
                {
                    highestId = outputPin.ID.Get();
                }
            }
        }

        for (Link* pLink : m_pSelectedEncounter->GetLinks())
        {
            if (pLink->StartPinID.Get() > highestId)
            {
                highestId = pLink->StartPinID.Get();
            }
            
            if (pLink->EndPinID.Get() > highestId)
            {
                highestId = pLink->EndPinID.Get();
            }
        }

        m_IdGenerator = EncounterEditorIdGenerator(highestId + 1);
    }
}

void EncounterEditor::UpdateEvents()
{
    if (m_pSelectedEncounter == nullptr || m_pSelectedEncounter->IsPlaying())
    {
        return;
    }

    if (ImGuiNodeEditor::BeginCreate())
    {
        ImGuiNodeEditor::PinId inputPinId, outputPinId;
        if (ImGuiNodeEditor::QueryNewLink(&inputPinId, &outputPinId))
        {
            // QueryNewLink returns true if editor want to create new link between pins.
            Pin* pInputPin = m_pSelectedEncounter->GetPin(inputPinId.Get());
            Pin* pOutputPin = m_pSelectedEncounter->GetPin(outputPinId.Get());

            if (pInputPin && pOutputPin) // both are valid, let's accept link
            {
                if (pInputPin->Type != pOutputPin->Type || pInputPin->Kind == pOutputPin->Kind)
                {
                    // The pins are of different types, so we reject the link.
                    ImGuiNodeEditor::RejectNewItem(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                }
                else if (ImGuiNodeEditor::AcceptNewItem()) // AcceptNewItem() returns true when the user releases the mouse button.
                {
                    // Since we accepted new link, lets add one to our list of links.
                    LinkUniquePtr pLink = std::make_unique<Link>(m_IdGenerator.GenerateId(), inputPinId, outputPinId);
                    m_pSelectedEncounter->AddLink(std::move(pLink));

                    // Draw new link.
                    //ImGuiNodeEditor::Link(m_Links.back().Id, m_Links.back().InputId, m_Links.back().OutputId);
                }
            }
        }
    }
    ImGuiNodeEditor::EndCreate(); // Wraps up object creation action handling.

    if (ImGuiNodeEditor::BeginDelete())
    {
        ImGuiNodeEditor::NodeId nodeId = 0;
        while (ImGuiNodeEditor::QueryDeletedNode(&nodeId))
        {
            if (ImGuiNodeEditor::AcceptDeletedItem())
            {
                m_pSelectedEncounter->RemoveNode(nodeId.Get());
            }
        }

        /*
        ImGuiNodeEditor::LinkId linkId = 0;
        while (ImGuiNodeEditor::QueryDeletedLink(&linkId))
        {
            if (ImGuiNodeEditor::AcceptDeletedItem())
            {
                auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                if (id != m_Links.end())
                    m_Links.erase(id);
            }
        }
        */
    }
    ImGuiNodeEditor::EndDelete();
}

void EncounterEditor::ResetEditor()
{
    m_LinkedPins.clear();
}

} // namespace WingsOfSteel::TheBrightestStar
