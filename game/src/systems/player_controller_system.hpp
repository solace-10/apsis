#pragma once

#include <array>
#include <optional>
#include <unordered_map>

#include <glm/vec2.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

#include "components/mech_navigation_component.hpp"

namespace WingsOfSteel
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

class PlayerControllerSystem : public System
{
public:
    PlayerControllerSystem(){};
    ~PlayerControllerSystem();

    void Initialize(Scene* pScene) override;
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
        InputCallbackToken pressed{ InputSystem::sInvalidInputCallbackToken };
        InputCallbackToken released{ InputSystem::sInvalidInputCallbackToken };
    };
    std::array<ActionPair, static_cast<size_t>(InputAction::InputActionCount)> m_InputActions;

    // Mouse aim
    InputCallbackToken m_MousePositionToken{ InputSystem::sInvalidInputCallbackToken };

    std::array<bool, 4> m_MovementDirections{ false, false, false, false };
    std::unordered_map<std::string, bool> m_WeaponActivations;

    glm::vec2 m_MousePosition{ 0.0f };
};

} // namespace WingsOfSteel