#include <core/log.hpp>
#include <pandora.hpp>

#include "ui/button.hpp"
#include "ui/dice.hpp"
#include "ui/divider.hpp"
#include "ui/heading.hpp"
#include "ui/image.hpp"
#include "ui/panel.hpp"
#include "ui/stack.hpp"
#include "ui/text.hpp"
#include "ui/ui.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

ElementSharedPtr CreateElement(const std::string& typeName)
{
    if (typeName == "Button")
    {
        return std::make_shared<Button>();
    }
    else if (typeName == "Dice")
    {
        return std::make_shared<Dice>();
    }
    else if (typeName == "Divider")
    {
        return std::make_shared<Divider>();
    }
    else if (typeName == "Heading")
    {
        return std::make_shared<Heading>();
    }
    else if (typeName == "Image")
    {
        return std::make_shared<Image>();
    }
    else if (typeName == "Panel")
    {
        return std::make_shared<Panel>();
    }
    else if (typeName == "Stack")
    {
        return std::make_shared<Stack>();
    }
    else if (typeName == "Text")
    {
        return std::make_shared<Text>();
    }
    else
    {
        Pandora::Log::Error() << "UI: don't know how to create element '" << typeName << "'.";
        return nullptr;
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI