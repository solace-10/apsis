#include <random>

#include "dice/dice_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

std::vector<Dice> DiceGenerator::Generate(size_t numDice)
{
    std::vector<Dice> dice;
    dice.reserve(numDice);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 6);
    
    for (size_t i = 0; i < numDice; ++i)
    {
        dice.emplace_back(dis(gen));
    }

    return dice;
}

} // namespace WingsOfSteel::TheBrightestStar