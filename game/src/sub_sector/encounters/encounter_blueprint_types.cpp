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
    data["position"] = {
        {"x", Position.x}, 
        {"y", Position.y}
    };
    
    data["inputs"] = nlohmann::json::array();
    for (const Pin& input : Inputs)
    {
        data["inputs"].push_back(input.ID.Get());
    }
    
    data["outputs"] = nlohmann::json::array();
    for (const Pin& output : Outputs)
    {
        data["outputs"].push_back(output.ID.Get());
    }

    return data;
}

void Node::Deserialize(const nlohmann::json& data)
{
    const auto idIt = data.find("id");
    if (idIt != data.end() && idIt->is_number_unsigned())
    {
        ID = idIt->get<uint32_t>();
    }

    const auto nameIt = data.find("name");
    if (nameIt != data.end() && nameIt->is_string())
    {
        Name = nameIt->get<std::string>();
    }

    const auto positionIt = data.find("position");
    if (positionIt != data.end() && positionIt->is_object())
    {
        const auto xIt = positionIt->find("x");
        const auto yIt = positionIt->find("y");
        if (xIt != positionIt->end() && yIt != positionIt->end() && xIt->is_number_float() && yIt->is_number_float())
        {
            Position = ImVec2(xIt->get<float>(), yIt->get<float>());
        }
    }

    const auto inputsIt = data.find("inputs");
    if (inputsIt != data.end() && inputsIt->is_array())
    {
        int a = 0;
    }
}

} // namespace WingsOfSteel::TheBrightestStar

