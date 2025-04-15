#pragma once

#include <optional>

#include <core/smart_ptr.hpp>

#include "dice/dice.hpp"
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
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void SetDice(const ::WingsOfSteel::TheBrightestStar::Dice& dice);
    void ClearDice();

private:
    std::optional<::WingsOfSteel::TheBrightestStar::Dice> m_Dice;
    float m_AnimationTimer{0.0f};
};

inline void Dice::SetDice(const ::WingsOfSteel::TheBrightestStar::Dice& dice)
{
    m_Dice = dice;
}

inline void Dice::ClearDice()
{
    m_Dice = std::nullopt;
}

} // namespace WingsOfSteel::TheBrightestStar::UI