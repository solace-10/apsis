#include "dice/dice.hpp"

namespace WingsOfSteel::TheBrightestStar
{

static Dice::Id s_DieId = 0;

Dice::Dice(DiceValue value) 
: m_Value(value)
{
    assert(value >= 1);
    assert(value <= 6);
    m_Id = s_DieId++;
}

} // namespace WingsOfSteel::TheBrightestStar