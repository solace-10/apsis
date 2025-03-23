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

nlohmann::json Node::Serialize() const
{
    nlohmann::json data;
    data["id"] = ID.Get();
    data["name"] = Name;
    return data;
}

void Node::Deserialize(const nlohmann::json& data)
{
    ID = data["id"].get<uint32_t>();
    Name = data["name"];
}

} // namespace WingsOfSteel::TheBrightestStar

