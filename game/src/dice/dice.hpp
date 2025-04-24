#pragma once

#include <cassert>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

using DiceValue = uint32_t;

enum class DiceCategory
{
    Electronics = 0,
    Engineering,
    Navigation,
    Science,
    Warfare
};

enum class DiceModifiers
{
    None = 0
};

DECLARE_SMART_PTR(Dice);
class Dice
{
public:
    Dice(DiceValue value);
    ~Dice() {}

    DiceValue GetValue() const;

    using Id = uint32_t;
    Id GetId() const;

private:
    DiceValue m_Value{1};
    DiceModifiers m_Modifiers{DiceModifiers::None};
    Id m_Id{0};
};

inline DiceValue Dice::GetValue() const
{
    return m_Value;
}

inline Dice::Id Dice::GetId() const
{
    return m_Id;
}

} // namespace WingsOfSteel::TheBrightestStar