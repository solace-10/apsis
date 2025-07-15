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
        pInputSystem->RemoveKeyboardCallback(m_ForwardButtonPressedToken);
        pInputSystem->RemoveKeyboardCallback(m_ForwardButtonReleasedToken);
        pInputSystem->RemoveKeyboardCallback(m_LeftButtonPressedToken);
        pInputSystem->RemoveKeyboardCallback(m_LeftButtonReleasedToken);
        pInputSystem->RemoveKeyboardCallback(m_RightButtonPressedToken);
        pInputSystem->RemoveKeyboardCallback(m_RightButtonReleasedToken);
        pInputSystem->RemoveKeyboardCallback(m_DownButtonPressedToken);
        pInputSystem->RemoveKeyboardCallback(m_DownButtonReleasedToken);
        pInputSystem->RemoveMousePositionCallback(m_MousePositionToken);
    }
}

void PlayerControllerSystem::Initialize(Pandora::Scene* pScene)
{
    using namespace Pandora;
    Pandora::InputSystem* pInputSystem = GetInputSystem();
    m_ForwardButtonPressedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Up, true); }, GLFW_KEY_W, KeyAction::Pressed);
    m_ForwardButtonReleasedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Up, false); }, GLFW_KEY_W, KeyAction::Released);
    m_LeftButtonPressedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Left, true); }, GLFW_KEY_A, KeyAction::Pressed);
    m_LeftButtonReleasedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Left, false); }, GLFW_KEY_A, KeyAction::Released);
    m_RightButtonPressedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Right, true); }, GLFW_KEY_D, KeyAction::Pressed);
    m_RightButtonReleasedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Right, false); }, GLFW_KEY_D, KeyAction::Released);
    m_DownButtonPressedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Down, true); }, GLFW_KEY_S, KeyAction::Pressed);
    m_DownButtonReleasedToken = pInputSystem->AddKeyboardCallback([this]() { SetMovementDirection(MovementDirection::Down, false); }, GLFW_KEY_S, KeyAction::Released);
    m_MousePositionToken = pInputSystem->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta) { m_MousePosition = mousePosition; });
}

void PlayerControllerSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();

    auto navigationView = registry.view<MechNavigationComponent, const PlayerControllerComponent>();
    navigationView.each([this](const auto entity, MechNavigationComponent& mechNavigationComponent, const PlayerControllerComponent& playerControllerComponent) {
        mechNavigationComponent.SetThrust(GetMovementDirection().value_or(glm::vec2(0.0f, 0.0f)));
    });

    const glm::vec3 targetWorldPos = Game::Get()->GetSector()->GetSystem<CameraSystem>()->MouseToWorld(m_MousePosition);
    auto weaponsView = registry.view<WeaponComponent>();
    weaponsView.each([targetWorldPos](const auto entity, WeaponComponent& weaponComponent) {
        EntitySharedPtr pParentShip = weaponComponent.m_pParent.lock();
        if (pParentShip && pParentShip->HasComponent<PlayerControllerComponent>())
        {
            weaponComponent.m_Target = targetWorldPos;
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

} // namespace WingsOfSteel::TheBrightestStar