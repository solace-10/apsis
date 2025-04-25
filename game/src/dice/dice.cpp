#include <cassert>

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

    switch (value)
    {
        case 1:
            m_PositiveChance = 0.0f;
            m_NeutralChance  = 0.5f;
            m_NegativeChance = 0.5f;
            break;
        case 2:
            m_PositiveChance = 0.1f;
            m_NeutralChance  = 0.4f;
            m_NegativeChance = 0.5f;
            break;
        case 3:
            m_PositiveChance = 0.3f;
            m_NeutralChance  = 0.3f;
            m_NegativeChance = 0.4f;
            break;
        case 4:
            m_PositiveChance = 0.4f;
            m_NeutralChance  = 0.3f;
            m_NegativeChance = 0.3f;
            break;
        case 5:
            m_PositiveChance = 0.5f;
            m_NeutralChance  = 0.4f;
            m_NegativeChance = 0.1f;
            break;
        case 6:
            m_PositiveChance = 0.5f;
            m_NeutralChance =  0.5f;
            m_NegativeChance = 0.0f;
            break;
        default:
            assert(false);
    }
}

} // namespace WingsOfSteel::TheBrightestStar