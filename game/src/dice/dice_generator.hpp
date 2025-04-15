#pragma once

#include <vector>

#include <core/smart_ptr.hpp>

#include "dice/dice.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class DiceGenerator
{
public:
    DiceGenerator() {}
    ~DiceGenerator() {}

    static std::vector<Dice> Generate(size_t numDice);
};

} // namespace WingsOfSteel::TheBrightestStar