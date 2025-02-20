#include "ui/image.hpp"
#include "ui/ui.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

ElementSharedPtr CreateElement(const std::string& typeName)
{
    if (typeName == "image")
    {
        return std::make_shared<Image>();
    }
    else
    {
        return nullptr;
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI