#include "encounter_blueprint_nodes.hpp"

namespace WingsOfSteel::TheBrightestStar
{

NodeUniquePtr BlueprintNodeFactory::CreateNode(const std::string& nodeName)
{
    if (nodeName == "Sector entered")
    {
        return std::make_unique<SectorEnteredNode>();
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

SectorEnteredNode::SectorEnteredNode()
: Node("Sector entered", ImColor(255, 0, 0))
{
    Outputs.emplace_back("", PinType::Flow);
}

EncounterStageNode::EncounterStageNode()
: Node("Encounter stage", ImColor(5, 250, 191))
{
    Inputs.emplace_back("", PinType::Flow);
    Inputs.emplace_back("Description", PinType::String);
    Outputs.emplace_back("Selected 1", PinType::Flow);
    Outputs.emplace_back("Selected 2", PinType::Flow);
    Outputs.emplace_back("Selected 3", PinType::Flow);
    Outputs.emplace_back("Dice", PinType::Dice);
}

StringNode::StringNode()
: Node("String", ImColor(124, 21, 153), NodeType::String)
{
    Outputs.emplace_back("Value", PinType::String);
}

nlohmann::json StringNode::Serialize() const
{
    nlohmann::json data = Node::Serialize();
    data["value"] = Value;
    return data;
}

void Deserialize(const nlohmann::json& data)
{
    if (data.contains("value"))
    {
        Value = data["value"].get<std::string>();
    }
}

} // namespace WingsOfSteel::TheBrightestStar
