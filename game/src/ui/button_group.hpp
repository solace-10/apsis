#pragma once

#include <vector>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Button);

DECLARE_SMART_PTR(ButtonGroup);
class ButtonGroup
{
public:
    ButtonGroup() {}
    ~ButtonGroup() {}

    void Add(const ButtonSharedPtr& pButton);
    void Select(Button* pButton);

private:
    std::vector<ButtonWeakPtr> m_Buttons;
};

} // namespace WingsOfSteel::TheBrightestStar::UI