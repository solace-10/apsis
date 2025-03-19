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

    StringNode* pStringNode = new StringNode();
    pStringNode->Initialize(m_IdGenerator);
    m_Nodes.emplace_back(pStringNode);
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

} // namespace WingsOfSteel::TheBrightestStar


