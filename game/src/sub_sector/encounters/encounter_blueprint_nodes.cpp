#include "encounter_blueprint_nodes.hpp"
#include "encounter.hpp"
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
    auto linkedNodes = pEncounter->GetLinkedNodes(&Outputs[0]);
    if (!linkedNodes.empty())
    {
        SetNextNode(linkedNodes[0]);
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
    Outputs.emplace_back("Selected 1", PinType::Flow);
    Outputs.emplace_back("Selected 2", PinType::Flow);
    Outputs.emplace_back("Selected 3", PinType::Flow);
    Outputs.emplace_back("Outcome", PinType::Outcome);
}

NodeType EncounterStageNode::GetNodeType() const
{
    return NodeType::EncounterStage;
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

} // namespace WingsOfSteel::TheBrightestStar
