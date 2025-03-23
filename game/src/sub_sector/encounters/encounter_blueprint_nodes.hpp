#pragma once

#include "encounter_blueprint_types.hpp"
#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class BlueprintNodeFactory
{
public:
    static NodeUniquePtr CreateNode(const std::string& nodeName);
};

class SectorEnteredNode : public Node
{
public:
    SectorEnteredNode();
};

class EncounterStageNode : public Node
{
public:
    EncounterStageNode();
};

class StringNode : public Node
{
public:
    StringNode();

    std::string Value;
};    

} // namespace WingsOfSteel::TheBrightestStar

