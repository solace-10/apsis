#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <imgui/imgui.hpp>

#include "encounter_blueprint_types.hpp"
#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(EncounterEditor);
class EncounterEditor
{
public:
    EncounterEditor();
    ~EncounterEditor();

    void Update();

private:
    void DrawNodes();
    void DrawStandardNode(Node* pNode);
    void DrawStringNode(Node* pNode);
    void DrawPins(Node* pNode);
    void DrawPinIcon(const Pin& pin, bool connected);
    ImColor GetIconColor(PinType type) const;
    int GetPinGroupWidth(const std::vector<Pin>& pins) const;
    int GetPinWidth(const Pin& pin) const;
    int GetPinIconSize() const;

    ImGuiNodeEditor::EditorContext* m_pContext{nullptr};
    std::vector<Node*> m_Nodes;
    EncounterEditorIdGenerator m_IdGenerator;
};

inline int EncounterEditor::GetPinIconSize() const
{
    return 24;
}

inline int EncounterEditor::GetPinWidth(const Pin& pin) const
{
    return static_cast<int>(ImGui::GetStyle().ItemSpacing.x) + GetPinIconSize() + ImGui::CalcTextSize(pin.Name.c_str()).x;
}

} // namespace WingsOfSteel::TheBrightestStar