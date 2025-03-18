#include "encounter_blueprint_nodes.hpp"

namespace WingsOfSteel::TheBrightestStar
{

SectorEnteredNode::SectorEnteredNode()
: Node(0, "Sector entered", ImColor(255, 0, 0))
{
    Outputs.emplace_back(Pin(0, "", PinType::Flow));
}

EncounterStageNode::EncounterStageNode()
: Node(0, "Encounter stage", ImColor(255, 0, 0))
{
    Inputs.emplace_back(Pin(0, "", PinType::Flow));
    Outputs.emplace_back(Pin(0, "Choice 1", PinType::Flow));
    Outputs.emplace_back(Pin(0, "Choice 2", PinType::Flow));
    Outputs.emplace_back(Pin(0, "Choice 3", PinType::Flow));
    Outputs.emplace_back(Pin(0, "Dice", PinType::Dice));
}


} // namespace WingsOfSteel::TheBrightestStar

