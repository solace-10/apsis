#include <glm/glm.hpp>

#include "encounter_editor.hpp"

#include "encounter_blueprint_nodes.hpp"
#include "encounter_editor_widgets.hpp"

namespace WingsOfSteel::TheBrightestStar
{

EncounterEditor::EncounterEditor()
{
    m_pContext = ImGuiNodeEditor::CreateEditor();

    SectorEnteredNode* pSectorEnteredNode = new SectorEnteredNode();
    pSectorEnteredNode->Initialize(m_IdGenerator);
    m_Nodes.emplace_back(pSectorEnteredNode);

    EncounterStageNode* pEncounterStageNode = new EncounterStageNode();
    pEncounterStageNode->Initialize(m_IdGenerator);
    m_Nodes.emplace_back(pEncounterStageNode);
}

EncounterEditor::~EncounterEditor()
{
    ImGuiNodeEditor::DestroyEditor(m_pContext);
}

void EncounterEditor::Update()
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Encounter Editor");
    ImGuiNodeEditor::SetCurrentEditor(m_pContext);
    ImGuiNodeEditor::Begin("Encounter Editor");

    DrawNodes();

    ImGuiNodeEditor::End();
    ImGuiNodeEditor::SetCurrentEditor(nullptr);

    ImGui::End();
}

void EncounterEditor::DrawNodes()
{
    const int nodeTitleHeight = 36;
    for (Node* pNode : m_Nodes)
    {
        ImGuiNodeEditor::BeginNode(pNode->ID);

        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImVec2 textSize = ImGui::CalcTextSize(pNode->Name.c_str());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
        ImGui::Dummy(ImVec2(textSize.x, nodeTitleHeight));
        ImGui::PopStyleVar();

        DrawPins(pNode);

        ImGui::SetCursorPos(cursorPos);
        
        ImVec2 nodeSize = ImGuiNodeEditor::GetNodeSize(pNode->ID);
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        ImVec2 nodePadding = ImVec2(8, 8);
        ImVec2 cp0 = ImGui::GetCursorScreenPos() - nodePadding + ImVec2(1, 1);
        ImVec2 cp1 = cp0 + ImVec2(nodeSize.x, nodeTitleHeight) - ImVec2(2, 2);
        pDrawList->AddRectFilled(cp0, cp1, IM_COL32(255, 0, 0, 120), 12.0, ImDrawFlags_RoundCornersTop);

        pDrawList->AddText(cp0 + ImVec2(8, 4), IM_COL32(255, 255, 255, 255), pNode->Name.c_str());

        ImGuiNodeEditor::EndNode();
    }
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
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
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

    ImVec2 outputPinsStartPos = pinsStartPos + ImVec2(glm::max(combinedPinsWidth, titleWidth), 0);
    int nodeWidth = ImGuiNodeEditor::GetNodeSize(pNode->ID).x;
    //outputPinsStartPos.x = glm::max(outputPinsStartPos.x, ImGuiNodeEditor::GetNodeSize(pNode->ID).x);
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
    ImGui::PopStyleVar();
}

void EncounterEditor::DrawPinIcon(const Pin& pin, bool connected)
{
    PinIconType iconType;
    ImColor color = ImColor(255, 255, 255); //GetIconColor(pin.Type);
    color.Value.w = 255 / 255.0f;
    switch (pin.Type)
    {
        case PinType::Flow: iconType = PinIconType::Flow; break;
        default: iconType = PinIconType::Circle; break;
    }

    const int pinIconSize = GetPinIconSize(); 
    PinIcon(ImVec2(pinIconSize, pinIconSize), iconType, connected, color, ImColor(32, 32, 32, 255));
}

} // namespace WingsOfSteel::TheBrightestStar

