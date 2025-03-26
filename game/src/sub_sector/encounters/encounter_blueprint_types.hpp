#pragma once

#include <vector>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>
#include <imgui/imgui.hpp>

#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Node);
DECLARE_SMART_PTR(Link);

enum class PinType
{
    Flow,
    Dice,
    String
};

enum class PinKind
{
    Output,
    Input
};

enum class NodeType
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
    ImGuiNodeEditor::PinId ID{0};
    Node* Node{nullptr};
    std::string Name;
    PinType Type{PinType::Flow};
    PinKind Kind{PinKind::Input};

    Pin(const char* name, PinType type):
        ID(-1), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
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

    ImGuiNodeEditor::NodeId ID{0};
    std::string Name;
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    ImColor Color{ImColor(255, 255, 255)};
    NodeType Type{NodeType::Standard};
    ImVec2 Size{0, 0};
    ImVec2 Position{0, 0};

    Node(const char* name, ImColor color = ImColor(255, 255, 255), NodeType type = NodeType::Standard):
        ID(-1), Name(name), Color(color), Type(type), Size(0, 0)
    {
    }

    virtual void Initialize(EncounterEditorIdGenerator& idGenerator);

    virtual nlohmann::json Serialize() const;
    virtual void Deserialize(const nlohmann::json& data);
    
    ExecutionResult Execute(float delta);
    std::optional<ImGuiNodeEditor::NodeId> GetNextNode() const;

private:
    void DeserializePins(std::vector<Pin>& pins, PinKind pinKind, const nlohmann::json& data);
};

struct Link
{
    ImGuiNodeEditor::LinkId ID{0};

    ImGuiNodeEditor::PinId StartPinID;
    ImGuiNodeEditor::PinId EndPinID;

    ImColor Color{ImColor(255, 255, 255)};

    Link(ImGuiNodeEditor::LinkId id, ImGuiNodeEditor::PinId startPinId, ImGuiNodeEditor::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};

} // namespace WingsOfSteel::TheBrightestStar

