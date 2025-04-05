#pragma once

#include <optional>
#include <vector>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>
#include <imgui/imgui.hpp>

#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Encounter);
DECLARE_SMART_PTR(Node);
DECLARE_SMART_PTR(Link);

// DO NOT CHANGE THE ORDER OF THESE ENUMS
enum class PinType
{
    Flow,
    Outcome,
    String,
    EncounterOption,
    Dice,
    Image
};

enum class PinKind
{
    Output,
    Input
};

enum class NodeType
{
    SectorEntered,
    SectorExit,
    EncounterStage,
    EncounterOption,
    String,
    Dice,
    Image
};

enum class NodeDisplayType
{
    Standard,
    String
};

enum class PinIconType
{ 
    Flow, 
    Circle, 
    Square, 
    Grid, 
    RoundSquare, 
    Diamond 
};


struct Pin
{
    ImGuiNodeEditor::PinId ID{InvalidBlueprintId};
    Node* Node{nullptr};
    std::string Name;
    PinType Type{PinType::Flow};
    PinKind Kind{PinKind::Input};
    bool Connected{false};

    Pin(const char* name, PinType type)
    : Name(name)
    , Type(type)
    {
    }
};

class Node
{
public:
    enum class ExecutionResult
    {
        Continue,
        Complete
    };

    ImGuiNodeEditor::NodeId ID{InvalidBlueprintId};
    std::string Name;
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    ImColor Color{ImColor(255, 255, 255)};
    NodeDisplayType Type{NodeDisplayType::Standard};
    ImVec2 Size{0, 0};
    ImVec2 Position{0, 0};

    Node(const char* name, ImColor color = ImColor(255, 255, 255), NodeDisplayType type = NodeDisplayType::Standard)
    : Name(name)
    , Color(color)
    , Type(type)
    {
    }

    virtual void Initialize(EncounterEditorIdGenerator& idGenerator);
    virtual NodeType GetNodeType() const = 0;
    virtual nlohmann::json Serialize() const;
    virtual void Deserialize(const nlohmann::json& data);

    virtual void OnExecutionStarted(Encounter* pEncounter);
    virtual ExecutionResult Execute(Encounter* pEncounter, float delta);
    virtual void OnExecutionEnded(Encounter* pEncounter);

    Node* GetNextNode() const;

protected:
    void SetNextNode(Node* pNextNode);

private:
    Node* m_pNextNode{nullptr};

    void DeserializePins(std::vector<Pin>& pins, PinKind pinKind, const nlohmann::json& data);
};

struct Link
{
    ImGuiNodeEditor::LinkId ID{InvalidBlueprintId};

    ImGuiNodeEditor::PinId StartPinID;
    ImGuiNodeEditor::PinId EndPinID;

    // Setting the link color is deferred until the editor actually needs to draw it.
    std::optional<ImColor> Color;

    Link(ImGuiNodeEditor::LinkId id, ImGuiNodeEditor::PinId startPinId, ImGuiNodeEditor::PinId endPinId)
    : ID(id)
    , StartPinID(startPinId)
    , EndPinID(endPinId)
    {
    }
};

} // namespace WingsOfSteel::TheBrightestStar

