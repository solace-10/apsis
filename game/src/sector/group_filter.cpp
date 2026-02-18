#include "sector/group_filter.hpp"

#include <cstdlib>

#include <core/log.hpp>

namespace WingsOfSteel
{

Color GroupFilter::HexToColor(const std::string& hexColor)
{
    if (hexColor.size() != 7 || hexColor[0] != '#')
    {
        Log::Error() << "Invalid hex color format: " << hexColor << ", expected #RRGGBB.";
        return Color(0.0f, 0.0f, 0.0f);
    }

    char* end = nullptr;
    long r = std::strtol(hexColor.substr(1, 2).c_str(), &end, 16);
    long g = std::strtol(hexColor.substr(3, 2).c_str(), &end, 16);
    long b = std::strtol(hexColor.substr(5, 2).c_str(), &end, 16);

    return Color(
        static_cast<float>(r) / 255.0f,
        static_cast<float>(g) / 255.0f,
        static_cast<float>(b) / 255.0f
    );
}

} // namespace WingsOfSteel
