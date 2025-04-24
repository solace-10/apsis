#pragma once

#include <optional>

#include <core/smart_ptr.hpp>

#include "dice/dice.hpp"
#include "ui/events.hpp"
#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Dice);
class Dice : public StackableElement
{
public:
    Dice();
    ~Dice() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;

    void SetOnClickedEvent(const OnClickedEvent& event);
    void SetDice(const ::WingsOfSteel::TheBrightestStar::Dice& dice);
    std::optional<::WingsOfSteel::TheBrightestStar::Dice> GetDice() const;
    void ClearDice();

private:
    std::optional<::WingsOfSteel::TheBrightestStar::Dice> m_Dice;
    float m_AnimationTimer{0.0f};
    OnClickedEvent m_OnClickedEvent;
};

inline void Dice::SetOnClickedEvent(const OnClickedEvent& event)
{
    m_OnClickedEvent = event;
}

inline void Dice::SetDice(const ::WingsOfSteel::TheBrightestStar::Dice& dice)
{
    m_Dice = dice;
}

inline std::optional<::WingsOfSteel::TheBrightestStar::Dice> Dice::GetDice() const
{
    return m_Dice;
}

inline void Dice::ClearDice()
{
    m_Dice = std::nullopt;
}

} // namespace WingsOfSteel::TheBrightestStar::UI