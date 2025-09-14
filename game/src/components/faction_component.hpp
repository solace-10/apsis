#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>
#include <magic_enum.hpp>

namespace WingsOfSteel
{

enum class Faction
{
    Allied,
    Hostile
};

class FactionComponent : public IComponent
{
public:
    FactionComponent() : Value(Faction::Hostile) {}
    FactionComponent(Faction faction) : Value(faction) {}
    ~FactionComponent() {}

    Faction Value;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["value"] = magic_enum::enum_name(Value);
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        std::string factionStr = DeserializeRequired<std::string>(json, "value");
        auto faction = magic_enum::enum_cast<Faction>(factionStr);
        if (faction.has_value())
        {
            Value = faction.value();
        }
        else
        {
            Value = Faction::Hostile; // Default fallback
        }
    }
};

REGISTER_COMPONENT(FactionComponent, "faction")

} // namespace WingsOfSteel