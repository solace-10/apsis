#include <GLFW/glfw3.h>

#include <cmath>
#include <limits>

#include <pandora.hpp>
#include <scene/scene.hpp>

#include "components/player_controller_component.hpp"
#include "components/mech_navigation_component.hpp"
#include "components/weapon_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/camera_system.hpp"
#include "systems/player_controller_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

PlayerControllerSystem::~PlayerControllerSystem()
{
    Pandora::InputSystem* pInputSystem = Pandora::GetInputSystem();
    if (pInputSystem)
    {
        for (auto& inputAction : m_InputActions)
        {
            pInputSystem->RemoveKeyboardCallback(inputAction.pressed);
            pInputSystem->RemoveKeyboardCallback(inputAction.released);
        }

        pInputSystem->RemoveMousePositionCallback(m_MousePositionToken);
    }
}

void PlayerControllerSystem::Initialize(Pandora::Scene* pScene)
{
    using namespace Pandora;
    Pandora::InputSystem* pInputSystem = GetInputSystem();

    m_InputActions[static_cast<size_t>(InputAction::Up)].pressed  = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Up, true); },  GLFW_KEY_W, KeyAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::Up)].released = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Up, false); }, GLFW_KEY_W, KeyAction::Released);

    m_InputActions[static_cast<size_t>(InputAction::Left)].pressed  = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Left, true); },  GLFW_KEY_A, KeyAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::Left)].released = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Left, false); }, GLFW_KEY_A, KeyAction::Released);

    m_InputActions[static_cast<size_t>(InputAction::Right)].pressed  = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Right, true); },  GLFW_KEY_D, KeyAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::Right)].released = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Right, false); }, GLFW_KEY_D, KeyAction::Released);

    m_InputActions[static_cast<size_t>(InputAction::Down)].pressed  = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Down, true); },  GLFW_KEY_S, KeyAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::Down)].released = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Down, false); }, GLFW_KEY_S, KeyAction::Released);

    m_InputActions[static_cast<size_t>(InputAction::LeftArmWeapon)].pressed  = pInputSystem->AddMouseButtonCallback([this]() { SetWeaponFire("LeftArm", true); },  MouseButton::Left, MouseAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::LeftArmWeapon)].released = pInputSystem->AddMouseButtonCallback([this]() { SetWeaponFire("LeftArm", false); }, MouseButton::Left, MouseAction::Released);

    m_InputActions[static_cast<size_t>(InputAction::RightArmWeapon)].pressed  = pInputSystem->AddMouseButtonCallback([this]() { SetWeaponFire("RightArm", true); },  MouseButton::Right, MouseAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::RightArmWeapon)].released = pInputSystem->AddMouseButtonCallback([this]() { SetWeaponFire("RightArm", false); }, MouseButton::Right, MouseAction::Released);

    m_InputActions[static_cast<size_t>(InputAction::LeftShoulderWeapon)].pressed  = pInputSystem->AddKeyboardCallback([this]() { SetWeaponFire("LeftShoulder", true); },  GLFW_KEY_Q, KeyAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::LeftShoulderWeapon)].released = pInputSystem->AddKeyboardCallback([this]() { SetWeaponFire("LeftShoulder", false); }, GLFW_KEY_Q, KeyAction::Released);

    m_InputActions[static_cast<size_t>(InputAction::RightShoulderWeapon)].pressed  = pInputSystem->AddKeyboardCallback([this]() { SetWeaponFire("RightShoulder", true); },  GLFW_KEY_E, KeyAction::Pressed);
    m_InputActions[static_cast<size_t>(InputAction::RightShoulderWeapon)].released = pInputSystem->AddKeyboardCallback([this]() { SetWeaponFire("RightShoulder", false); }, GLFW_KEY_E, KeyAction::Released);

    m_MousePositionToken = pInputSystem->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta) { m_MousePosition = mousePosition; });
}

void PlayerControllerSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();

    const glm::vec3 targetWorldPos = Game::Get()->GetSector()->GetSystem<CameraSystem>()->MouseToWorld(m_MousePosition);

    auto navigationView = registry.view<MechNavigationComponent, const PlayerControllerComponent>();
    navigationView.each([this, targetWorldPos](const auto entity, MechNavigationComponent& mechNavigationComponent, const PlayerControllerComponent& playerControllerComponent) {
        const std::optional<glm::vec2> movementDirection = GetMovementDirection();
        if (movementDirection.has_value())
        {
            const glm::vec2 md(movementDirection.value());
            const glm::vec3 thrustDirection(md.x, 0.0f, -md.y);
            mechNavigationComponent.SetThrust(thrustDirection);
        }
        else
        {
            mechNavigationComponent.ClearThrust();
        }

        mechNavigationComponent.SetAim(targetWorldPos);
    });

    auto weaponsView = registry.view<WeaponComponent>();
    weaponsView.each([this, targetWorldPos](const auto entity, WeaponComponent& weaponComponent) {
        EntitySharedPtr pParentShip;
        EntitySharedPtr pOwnerEntity = weaponComponent.GetOwner().lock();
        if (pOwnerEntity)
        {
            pParentShip = pOwnerEntity->GetParent().lock();
            if (pParentShip && pParentShip->HasComponent<PlayerControllerComponent>())
            {
                weaponComponent.m_Target = targetWorldPos;

                auto it = m_WeaponActivations.find(weaponComponent.m_AttachmentPointName);
                if (it == m_WeaponActivations.cend())
                {
                    weaponComponent.m_WantsToFire = false;
                }
                else
                {
                    weaponComponent.m_WantsToFire = it->second;
                }
            }
        }
    });
}

void PlayerControllerSystem::SetMovementDirection(MovementDirection direction, bool state)
{
    m_MovementDirections[static_cast<size_t>(direction)] = state;
}

std::optional<glm::vec2> PlayerControllerSystem::GetMovementDirection() const
{
    glm::vec2 direction(0.0f);
    
    if (m_MovementDirections[static_cast<size_t>(MovementDirection::Up)])
        direction.y += 1.0f;
    if (m_MovementDirections[static_cast<size_t>(MovementDirection::Down)])
        direction.y -= 1.0f;
    if (m_MovementDirections[static_cast<size_t>(MovementDirection::Left)])
        direction.x -= 1.0f;
    if (m_MovementDirections[static_cast<size_t>(MovementDirection::Right)])
        direction.x += 1.0f;
    
    if (std::abs(direction.x) < std::numeric_limits<float>::epsilon() && 
        std::abs(direction.y) < std::numeric_limits<float>::epsilon())
        return std::nullopt;
    
    return glm::normalize(direction);
}

void PlayerControllerSystem::SetWeaponFire(const std::string& weaponAttachment, bool state)
{
    m_WeaponActivations[weaponAttachment] = state;
}

} // namespace WingsOfSteel::TheBrightestStar