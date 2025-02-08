#include <string>

#include <imgui.h>

#include "sub_sector/encounters/encounter_window.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void EncounterWindow::Initialize()
{
    Window::Initialize();

    m_pEncounterImage = std::make_shared<UI::Image>();
    Bind("image/x", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetX(std::get<int>(value)); });
    Bind("image/y", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetY(std::get<int>(value)); });
    Bind("image/w", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetWidth(std::get<int>(value)); });
    Bind("image/h", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetHeight(std::get<int>(value)); });
    //Bind("image/path", [this](const UI::PrefabDataValue& value) { m_pEncounterImage->SetPath(std::get<std::string>(value)); });
    AddElement(m_pEncounterImage);
}

void EncounterWindow::Render()
{
    Window::Render();
}

} // namespace WingsOfSteel::TheBrightestStar