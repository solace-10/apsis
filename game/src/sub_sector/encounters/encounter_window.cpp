#include <string>

#include <imgui.h>

#include "sub_sector/encounters/encounter_window.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void EncounterWindow::Initialize(const std::string& prefabPath)
{
    Window::Initialize(prefabPath);

    /*
    m_pEncounterImage = std::make_shared<UI::Image>();
    Bind("image/x", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetX(std::get<int>(value)); });
    Bind("image/y", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetY(std::get<int>(value)); });
    Bind("image/w", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetWidth(std::get<int>(value)); });
    Bind("image/h", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetHeight(std::get<int>(value)); });
    Bind("image/path", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetPath(std::get<std::string>(value)); });
    AddElement(m_pEncounterImage);

    m_pEncounterText = std::make_shared<UI::Text>();
    Bind("encounter_text/text", [this](const UI::PrefabDataValue& value) { m_pEncounterText->SetText(std::get<std::string>(value)); });
    Bind("encounter_text/x", [this](const UI::PrefabDataValue& value) { m_pEncounterText->SetX(std::get<int>(value)); });
    Bind("encounter_text/y", [this](const UI::PrefabDataValue& value) { m_pEncounterText->SetY(std::get<int>(value)); });
    Bind("encounter_text/w", [this](const UI::PrefabDataValue& value) { m_pEncounterText->SetWidth(std::get<int>(value)); });
    Bind("encounter_text/h", [this](const UI::PrefabDataValue& value) { m_pEncounterText->SetHeight(std::get<int>(value)); });
    AddElement(m_pEncounterText);
    */
}


} // namespace WingsOfSteel::TheBrightestStar