#include <core/log.hpp>
#include <pandora.hpp>

#include "ui/encounter_button.hpp"
#include "ui/image.hpp"
#include "ui/stack.hpp"
#include "ui/text.hpp"
#include "ui/ui.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

ElementSharedPtr CreateElement(const std::string& typeName)
{
    if (typeName == "Image")
    {
        return std::make_shared<Image>();
    }
    else if (typeName == "Stack")
    {
        return std::make_shared<Stack>();
    }
    else if (typeName == "Text")
    {
        return std::make_shared<Text>();
    }
    else if (typeName == "EncounterButton")
    {
        return std::make_shared<EncounterButton>();
    }
    else
    {
        Pandora::Log::Error() << "UI: don't know how to create element '" << typeName << "'.";
        return nullptr;
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI