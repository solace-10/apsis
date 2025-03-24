#include <core/log.hpp>
#include <pandora.hpp>

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
        nlohmann::json pinData = {
            {"id", input.ID.Get()},
            {"name", input.Name},
            {"type", input.Type}
        };

        data["inputs"].push_back(pinData);
    }
    
    data["outputs"] = nlohmann::json::array();
    for (const Pin& output : Outputs)
    {
        nlohmann::json pinData = {
            {"id", output.ID.Get()},
            {"name", output.Name},
            {"type", output.Type}
        };

        data["outputs"].push_back(pinData);
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
    if (inputsIt != data.end())
    {
        DeserializePins(Inputs, *inputsIt);
    }

    const auto outputsIt = data.find("outputs");
    if (outputsIt != data.end())
    {
        DeserializePins(Outputs, *outputsIt);
    }
}

void Node::DeserializePins(std::vector<Pin>& pins, const nlohmann::json& data)
{
    if (!data.is_array() || pins.empty())
    {
        return;
    }

    for (const auto& pin : data)
    {
        if (pin.is_object())
        {
            const auto idIt = pin.find("id");
            const auto nameIt = pin.find("name");
            const auto typeIt = pin.find("type");

            if (idIt != pin.end() && idIt->is_number_unsigned() &&
                nameIt != pin.end() && nameIt->is_string() &&
                typeIt != pin.end() && typeIt->is_number_unsigned())
            {
                const uint32_t id = idIt->get<uint32_t>();
                const std::string name = nameIt->get<std::string>();
                const PinType type = static_cast<PinType>(typeIt->get<uint32_t>());

                bool pinAssigned = false;
                for (auto& pin : pins)
                {
                    if (pin.Name == name && pin.Type == type)
                    {
                        pin.ID = id;
                        pin.Node = this;
                        pinAssigned = true;
                        break;
                    }
                }

                if (!pinAssigned)
                {
                    Pandora::Log::Warning() << "Pin not assigned (not found or name/type mismatch): " << name;
                }
            }
            else
            {
                Pandora::Log::Warning() << "Invalid pin data.";
            }
        }
        else
        {
            Pandora::Log::Warning() << "Invalid pin data.";
        }
    }
}

} // namespace WingsOfSteel::TheBrightestStar

