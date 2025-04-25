#pragma once

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

    float GetPositiveChance() const;
    float GetNeutralChance() const;
    float GetNegativeChance() const;

private:
    DiceValue m_Value{1};
    DiceModifiers m_Modifiers{DiceModifiers::None};
    Id m_Id{0};
    float m_PositiveChance{0.0f};
    float m_NeutralChance{0.0f};
    float m_NegativeChance{0.0f};
};

inline DiceValue Dice::GetValue() const
{
    return m_Value;
}

inline Dice::Id Dice::GetId() const
{
    return m_Id;
}

inline float Dice::GetPositiveChance() const
{
    return m_PositiveChance;
}

inline float Dice::GetNeutralChance() const
{
    return m_NeutralChance;
}

inline float Dice::GetNegativeChance() const
{
    return m_NegativeChance;
}

} // namespace WingsOfSteel::TheBrightestStar