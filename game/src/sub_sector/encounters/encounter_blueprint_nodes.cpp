#include "encounter_blueprint_nodes.hpp"

namespace WingsOfSteel::TheBrightestStar
{

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

} // namespace WingsOfSteel::TheBrightestStar

