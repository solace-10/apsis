#pragma once

#include "ui/window.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(VictoryWindow);
class VictoryWindow : public UI::Window
{
public:
    VictoryWindow();
    ~VictoryWindow() override;

    void OnInitializationCompleted() override;
};

} // namespace WingsOfSteel
