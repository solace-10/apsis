#pragma once

#include <core/smart_ptr.hpp>

#include "ui/image.hpp"
#include "ui/text.hpp"
#include "ui/window.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(EncounterWindow);
class EncounterWindow : public UI::Window
{
public:
    EncounterWindow() {}
    ~EncounterWindow() override {}

    void OnInitializationCompleted() override;

    void AppendText(const std::string& text);

private:
    //UI::ImageSharedPtr m_pEncounterImage;
    UI::TextSharedPtr m_pEncounterText;
};

} // namespace WingsOfSteel::TheBrightestStar