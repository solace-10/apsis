#pragma once

#include <string>

namespace WingsOfSteel::TheBrightestStar
{

class NameComponent
{
public:
    NameComponent() {}
    NameComponent(const std::string& name) : Value(name) {}
    ~NameComponent() {}

    std::string Value;
};

} // namespace WingsOfSteel::TheBrightestStar