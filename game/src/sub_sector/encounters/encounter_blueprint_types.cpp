#include "encounter_blueprint_types.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void Node::Initialize(EncounterEditorIdGenerator& idGenerator)
{
    ID = idGenerator.GenerateId();

    for (Pin& input : Inputs)
    {
        input.ID = idGenerator.GenerateId();
    }

    for (Pin& output : Outputs)
    {
        output.ID = idGenerator.GenerateId();
    }
}

} // namespace WingsOfSteel::TheBrightestStar


