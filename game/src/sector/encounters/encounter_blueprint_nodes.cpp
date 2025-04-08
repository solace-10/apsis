#include <imgui/text_editor/text_editor.hpp>

#include "sector/encounters/encounter_window.hpp"
#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "sector/encounters/encounter.hpp"

namespace WingsOfSteel::TheBrightestStar
{

NodeUniquePtr BlueprintNodeFactory::CreateNode(const std::string& nodeName)
{
    if (nodeName == "Sector entered")
    {
        return std::make_unique<SectorEnteredNode>();
    }
    else if (nodeName == "Exit sector")
    {
        return std::make_unique<SectorExitNode>();
    }
    else if (nodeName == "Encounter stage")
    {
        return std::make_unique<EncounterStageNode>();
    }
    else if (nodeName == "Encounter option")
    {
        return std::make_unique<EncounterOptionNode>();
    }
    else if (nodeName == "Dice")
    {
        return std::make_unique<DiceNode>();
    }
    else if (nodeName == "Image")
    {
        return std::make_unique<ImageNode>();
    }
    else if (nodeName == "String")
    {
        return std::make_unique<StringNode>();
    }

    return nullptr;
}

////////////////////////////////////////////////////////////
// Sector entered
////////////////////////////////////////////////////////////

SectorEnteredNode::SectorEnteredNode()
: Node("Sector entered", ImColor(255, 0, 0))
{
    Outputs.emplace_back("", PinType::Flow);
}

NodeType SectorEnteredNode::GetNodeType() const
{
    return NodeType::SectorEntered;
}

Node::ExecutionResult SectorEnteredNode::Execute(Encounter* pEncounter, float delta)
{
    Node* pLinkedNode = pEncounter->GetFirstLinkedNode(&Outputs[0], true);
    if (pLinkedNode)
    {
        SetNextNode(pLinkedNode);
    }

    return ExecutionResult::Complete;
}

////////////////////////////////////////////////////////////
// Sector exit
////////////////////////////////////////////////////////////

SectorExitNode::SectorExitNode()
: Node("Exit sector", ImColor(255, 0, 0))
{
    Inputs.emplace_back("", PinType::Flow);
}

NodeType SectorExitNode::GetNodeType() const
{
    return NodeType::SectorExit;
}

////////////////////////////////////////////////////////////
// Encounter stage
////////////////////////////////////////////////////////////

EncounterStageNode::EncounterStageNode()
: Node("Encounter stage", ImColor(5, 250, 191))
{
    Inputs.emplace_back("", PinType::Flow);
    Inputs.emplace_back("Description", PinType::String);
    Inputs.emplace_back("Image", PinType::Image);
    Inputs.emplace_back("Option 1", PinType::EncounterOption);
    Inputs.emplace_back("Option 2", PinType::EncounterOption);
    Inputs.emplace_back("Option 3", PinType::EncounterOption);
    Outputs.emplace_back("Selected 1", PinType::Flow);
    Outputs.emplace_back("Selected 2", PinType::Flow);
    Outputs.emplace_back("Selected 3", PinType::Flow);
    Outputs.emplace_back("Outcome", PinType::Outcome);
}

NodeType EncounterStageNode::GetNodeType() const
{
    return NodeType::EncounterStage;
}

void EncounterStageNode::OnExecutionStarted(Encounter* pEncounter)
{
    auto pEncounterWindow = pEncounter->GetEncounterWindow();
    if (pEncounterWindow)
    {
        auto pLinkedNode = pEncounter->GetFirstLinkedNode(&Inputs[1]);
        if (pLinkedNode && pLinkedNode->GetNodeType() == NodeType::String)
        {
            auto pStringNode = static_cast<StringNode*>(pLinkedNode);
            pEncounterWindow->AppendText(pStringNode->Value);
        }
    }
}

Node::ExecutionResult EncounterStageNode::Execute(Encounter* pEncounter, float delta)
{
    return Node::ExecutionResult::Continue;
}

////////////////////////////////////////////////////////////
// Encounter option
////////////////////////////////////////////////////////////

EncounterOptionNode::EncounterOptionNode()
: Node("Encounter option", ImColor(5, 250, 191))
{
    Inputs.emplace_back("Dice", PinType::Dice);
    Inputs.emplace_back("Description", PinType::String);
    Outputs.emplace_back("Option", PinType::EncounterOption);
}

NodeType EncounterOptionNode::GetNodeType() const
{
    return NodeType::EncounterOption;
}

////////////////////////////////////////////////////////////
// String
////////////////////////////////////////////////////////////

StringNode::StringNode()
: Node("String", ImColor(124, 21, 153), NodeDisplayType::String)
{
    Outputs.emplace_back("Value", PinType::String);
}

NodeType StringNode::GetNodeType() const
{
    return NodeType::String;
}

nlohmann::json StringNode::Serialize() const
{
    nlohmann::json data = Node::Serialize();
    data["value"] = Value;
    return data;
}

void StringNode::Deserialize(const nlohmann::json& data)
{
    Node::Deserialize(data);
    auto valueIt = data.find("value");
    if (valueIt != data.end() && valueIt->is_string())
    {
        Value = valueIt->get<std::string>();
    }
}

////////////////////////////////////////////////////////////
// Dice
////////////////////////////////////////////////////////////

DiceNode::DiceNode()
: Node("Dice", ImColor(255, 165, 0))
{
    Outputs.emplace_back("Value", PinType::Dice);
}

NodeType DiceNode::GetNodeType() const
{
    return NodeType::Dice;
}

////////////////////////////////////////////////////////////
// Image
////////////////////////////////////////////////////////////

ImageNode::ImageNode()
: Node("Image", ImColor(165, 255, 0))
{
    Outputs.emplace_back("Value", PinType::Image);
}

NodeType ImageNode::GetNodeType() const
{
    return NodeType::Image;
}

} // namespace WingsOfSteel::TheBrightestStar
