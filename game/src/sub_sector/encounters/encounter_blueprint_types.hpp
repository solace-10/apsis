#pragma once

#include <vector>

#include <imgui/imgui.hpp>

#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

enum class PinType
{
    Flow,
    Dice
};

enum class PinKind
{
    Output,
    Input
};

enum class NodeType
{
    Blueprint
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

struct Node;

struct Pin
{
    ImGuiNodeEditor::PinId ID{0};
    Node* Node{nullptr};
    std::string Name;
    PinType Type{PinType::Flow};
    PinKind Kind{PinKind::Input};

    Pin(int id, const char* name, PinType type):
        ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
    {
    }
};

class Node
{
public:
    ImGuiNodeEditor::NodeId ID{0};
    std::string Name;
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    ImColor Color{ImColor(255, 255, 255)};
    NodeType Type{NodeType::Blueprint};
    ImVec2 Size{0, 0};

    Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)):
        ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
    {
    }

    virtual void Initialize(EncounterEditorIdGenerator& idGenerator);
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

