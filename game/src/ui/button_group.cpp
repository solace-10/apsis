#include "ui/button.hpp"
#include "ui/button_group.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

void ButtonGroup::Add(const ButtonSharedPtr& pButton)
{
    m_Buttons.push_back(pButton);
}
    
void ButtonGroup::Select(Button* pButton)
{
    for (auto& pGroupButtonWeakPtr : m_Buttons)
    {
        ButtonSharedPtr pGroupButton = pGroupButtonWeakPtr.lock();
        if (pGroupButton)
        {
            if (pButton && pButton == pGroupButton.get())
            {
                pGroupButton->AddFlag(Element::Flags::Selected);
            }
            else 
            {
                pGroupButton->RemoveFlag(Element::Flags::Selected);
            }
        }
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI