#include <GLFW/glfw3.h>

#include <scene/scene.hpp>
#include <pandora.hpp>

#include "components/player_controller_component.hpp"
#include "components/ship_navigation_component.hpp"
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
    }
}

void PlayerControllerSystem::Initialize(Pandora::Scene* pScene)
{
    using namespace Pandora;
    m_ForwardButtonPressedToken = GetInputSystem()->AddKeyboardCallback([this](){ this->m_Thrust = 1.0f; }, GLFW_KEY_W, KeyAction::Pressed );
    m_ForwardButtonReleasedToken = GetInputSystem()->AddKeyboardCallback([this](){ this->m_Thrust = 0.0f; }, GLFW_KEY_W, KeyAction::Released );
    m_LeftButtonPressedToken = GetInputSystem()->AddKeyboardCallback([this](){ this->m_SteerCommand = ShipSteerCommand::Port; }, GLFW_KEY_A, KeyAction::Pressed );
    m_LeftButtonReleasedToken = GetInputSystem()->AddKeyboardCallback([this](){ this->m_SteerCommand = ShipSteerCommand::None; }, GLFW_KEY_A, KeyAction::Released );
    m_RightButtonPressedToken = GetInputSystem()->AddKeyboardCallback([this](){ this->m_SteerCommand = ShipSteerCommand::Starboard; }, GLFW_KEY_D, KeyAction::Pressed );
    m_RightButtonReleasedToken = GetInputSystem()->AddKeyboardCallback([this](){ this->m_SteerCommand = ShipSteerCommand::None; }, GLFW_KEY_D, KeyAction::Released );
}

void PlayerControllerSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<ShipNavigationComponent, const PlayerControllerComponent>();

    view.each([this](const auto entity, ShipNavigationComponent& shipNavigationComponent, const PlayerControllerComponent& playerControllerComponent)
    {
        shipNavigationComponent.SetThrust(m_Thrust);
        shipNavigationComponent.SetSteerCommand(m_SteerCommand);
    });
}

} // namespace WingsOfSteel::TheBrightestStar