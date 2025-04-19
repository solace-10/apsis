#pragma once

#include <array>
#include <optional>

#include <magic_enum.hpp>

#include <core/smart_ptr.hpp>

#include "dice/dice.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class DiceComponent
{
public:
    DiceComponent() {}
    ~DiceComponent() {}

    using DiceContainer = std::array<std::optional<Dice>, 2>;

    DiceContainer& GetDice(DiceCategory category);
    const DiceContainer& GetDice(DiceCategory category) const;

private:
    std::array<DiceContainer, magic_enum::enum_count<DiceCategory>()> m_Dice;
};

inline DiceComponent::DiceContainer& DiceComponent::GetDice(DiceCategory category)
{
    return m_Dice[static_cast<size_t>(category)];
}

inline const DiceComponent::DiceContainer& DiceComponent::GetDice(DiceCategory category) const
{
    return m_Dice[static_cast<size_t>(category)];
}

} // namespace WingsOfSteel::TheBrightestStar