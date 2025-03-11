#include <core/log.hpp>
#include <pandora.hpp>

#include "ui/image.hpp"
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