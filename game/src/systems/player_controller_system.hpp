#pragma once

#include <array>
#include <optional>
#include <unordered_map>

#include <glm/vec2.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

#include "components/mech_navigation_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

enum class InputAction
{
    Up,
    Down,
    Left,
    Right,
    LeftArmWeapon,
    RightArmWeapon,
    LeftShoulderWeapon,
    RightShoulderWeapon,

    InputActionCount
};

class PlayerControllerSystem : public Pandora::System
{
public:
    PlayerControllerSystem(){};
    ~PlayerControllerSystem();

    void Initialize(Pandora::Scene* pScene) override;
    void Update(float delta) override;

private:
    enum class MovementDirection
    {
        Up = 0,
        Down,
        Left,
        Right
    };
    void SetMovementDirection(MovementDirection direction, bool state);
    std::optional<glm::vec2> GetMovementDirection() const;

    void SetWeaponFire(const std::string& weaponAttachment, bool state);

    struct ActionPair
    {
        Pandora::InputCallbackToken pressed{ Pandora::InputSystem::sInvalidInputCallbackToken };
        Pandora::InputCallbackToken released{ Pandora::InputSystem::sInvalidInputCallbackToken };
    };
    std::array<ActionPair, static_cast<size_t>(InputAction::InputActionCount)> m_InputActions;

    // Mouse aim
    Pandora::InputCallbackToken m_MousePositionToken{ Pandora::InputSystem::sInvalidInputCallbackToken };

    std::array<bool, 4> m_MovementDirections{ false, false, false, false };
    std::unordered_map<std::string, bool> m_WeaponActivations;

    glm::vec2 m_MousePosition{ 0.0f };
};

} // namespace WingsOfSteel::TheBrightestStar