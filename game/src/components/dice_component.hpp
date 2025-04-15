#pragma once

#include <array>
#include <optional>

#include <core/smart_ptr.hpp>

#include "dice/dice.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class DiceComponent
{
public:
    DiceComponent() {}
    ~DiceComponent() {}

    std::array<std::array<std::optional<Dice>, 2>, static_cast<size_t>(DiceCategory::Count)> Dice;
};

} // namespace WingsOfSteel::TheBrightestStar