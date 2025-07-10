#include <GLFW/glfw3.h>

#include <pandora.hpp>
#include <scene/scene.hpp>

#include "components/player_controller_component.hpp"
#include "components/ship_navigation_component.hpp"
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
        pInputSystem->RemoveMousePositionCallback(m_MousePositionToken);
    }
}

void PlayerControllerSystem::Initialize(Pandora::Scene* pScene)
{
    using namespace Pandora;
    Pandora::InputSystem* pInputSystem = GetInputSystem();
    m_ForwardButtonPressedToken = pInputSystem->AddKeyboardCallback([this]() { m_Thrust = ShipThrust::Forward; }, GLFW_KEY_W, KeyAction::Pressed);
    m_ForwardButtonReleasedToken = pInputSystem->AddKeyboardCallback([this]() { m_Thrust = ShipThrust::None; }, GLFW_KEY_W, KeyAction::Released);
    m_LeftButtonPressedToken = pInputSystem->AddKeyboardCallback([this]() { m_Steer = ShipSteer::Port; }, GLFW_KEY_A, KeyAction::Pressed);
    m_LeftButtonReleasedToken = pInputSystem->AddKeyboardCallback([this]() { m_Steer = ShipSteer::None; }, GLFW_KEY_A, KeyAction::Released);
    m_RightButtonPressedToken = pInputSystem->AddKeyboardCallback([this]() { m_Steer = ShipSteer::Starboard; }, GLFW_KEY_D, KeyAction::Pressed);
    m_RightButtonReleasedToken = pInputSystem->AddKeyboardCallback([this]() { m_Steer = ShipSteer::None; }, GLFW_KEY_D, KeyAction::Released);
    m_MousePositionToken = pInputSystem->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta) { m_MousePosition = mousePosition; });
}

void PlayerControllerSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();

    auto navigationView = registry.view<ShipNavigationComponent, const PlayerControllerComponent>();
    navigationView.each([this](const auto entity, ShipNavigationComponent& shipNavigationComponent, const PlayerControllerComponent& playerControllerComponent) {
        shipNavigationComponent.SetThrust(m_Thrust);
        shipNavigationComponent.SetSteer(m_Steer);
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

} // namespace WingsOfSteel::TheBrightestStar