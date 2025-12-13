#pragma once

#include "ui/window.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(DefeatWindow);
class DefeatWindow : public UI::Window
{
public:
    DefeatWindow();
    ~DefeatWindow() override;

    void OnInitializationCompleted() override;
};

} // namespace WingsOfSteel
