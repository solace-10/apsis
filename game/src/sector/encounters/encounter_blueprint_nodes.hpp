#pragma once

#include <core/smart_ptr.hpp>

#include "encounter_blueprint_types.hpp"
#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::Pandora
{
DECLARE_SMART_PTR(TextEditor);
}

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

    NodeType GetNodeType() const override;
    ExecutionResult Execute(Encounter* pEncounter, float delta) override;
};

class SectorExitNode : public Node
{
public:
    SectorExitNode();

    NodeType GetNodeType() const override;
};

class EncounterStageNode : public Node
{
public:
    EncounterStageNode();

    NodeType GetNodeType() const override;

    void OnExecutionStarted(Encounter* pEncounter) override;
    ExecutionResult Execute(Encounter* pEncounter, float delta) override;
};

class EncounterOptionNode : public Node
{
public:
    EncounterOptionNode();

    NodeType GetNodeType() const override;
};

class StringNode : public Node
{
public:
    StringNode();

    NodeType GetNodeType() const override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    std::string Value;
    Pandora::TextEditorUniquePtr Editor;
};

class DiceNode : public Node
{
public:
    DiceNode();

    NodeType GetNodeType() const override;
};

class ImageNode : public Node
{
public:
    ImageNode();

    NodeType GetNodeType() const override;
};

} // namespace WingsOfSteel::TheBrightestStar

