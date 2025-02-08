#pragma once

#include <core/smart_ptr.hpp>

#include "ui/image.hpp"
#include "ui/window.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(EncounterWindow);
class EncounterWindow : public UI::Window
{
public:
    EncounterWindow() {}
    ~EncounterWindow() override {}

    void Initialize() override;
    void Render() override;

private:
    UI::ImageSharedPtr m_pEncounterImage;
};

} // namespace WingsOfSteel::TheBrightestStar