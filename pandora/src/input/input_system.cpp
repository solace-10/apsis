#include <GLFW/glfw3.h>

#include "input/input_system.hpp"
#include "render/window.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

InputCallbackToken InputSystem::m_sToken = 1u;
InputCallbackToken InputSystem::sInvalidInputCallbackToken = 0u;

InputSystem::InputSystem()
{
}

InputSystem::~InputSystem()
{
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();
    glfwSetKeyCallback(pWindow, nullptr);
    glfwSetCursorPosCallback(pWindow, nullptr);
    glfwSetScrollCallback(pWindow, nullptr);
}

void InputSystem::Initialize()
{
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();

    glfwSetKeyCallback(pWindow, [](GLFWwindow* pWindow, int key, int scancode, int action, int mods){
        GetInputSystem()->HandleKeyboardEvent(key, scancode, action, mods);
    });

    glfwSetCursorPosCallback(pWindow, [](GLFWwindow* pWindow, double xPos, double yPos){
        GetInputSystem()->m_MousePosition = glm::vec2(
            static_cast<float>(xPos),
            static_cast<float>(yPos)
        );
    });

    glfwSetScrollCallback(pWindow, [](GLFWwindow* pWindow, double xOffset, double yOffset){
        GetInputSystem()->HandleMouseWheelEvent(xOffset, yOffset);
    });
}

void InputSystem::HandleMouseWheelEvent(double xOffset, double yOffset)
{
    for ( auto& callbackInfo : m_MouseWheelCallbacks )
    {
        callbackInfo.callback(static_cast<float>(xOffset), static_cast<float>(yOffset));
    }
}

void InputSystem::HandleKeyboardEvent(int key, int scancode, int action, int mods)
{
    for (auto& callbackInfo : m_KeyboardCallbacks)
    {
        if (callbackInfo.key == key)
        {
            if ((callbackInfo.action == KeyAction::Pressed && action == GLFW_PRESS) ||
                (callbackInfo.action == KeyAction::Released && action == GLFW_RELEASE) ||
                (callbackInfo.action == KeyAction::Held && action == GLFW_REPEAT))
            {
                callbackInfo.callback();
            }
        }
    }
}

void InputSystem::HandleMouseButtonEvent(int button, int scancode, int action, int mods)
{
    for (auto& callbackInfo : m_MouseCallbacks)
    {
        if (static_cast<int>(callbackInfo.button) == button)
        {
            if ((callbackInfo.action == MouseAction::Pressed && action == GLFW_PRESS) ||
                (callbackInfo.action == MouseAction::Released && action == GLFW_RELEASE))
            {
                callbackInfo.callback();
            }
        }
    }
}

InputCallbackToken InputSystem::AddKeyboardCallback(InputCallbackKeyboardFn callback, int key, KeyAction action)
{
    InputCallbackKeyboardInfo info;
    info.callback = callback;
    info.key = key;
    info.action = action;
    info.token = GenerateToken();
    m_KeyboardCallbacks.push_back(info);
    return info.token;
}

InputCallbackToken InputSystem::AddMouseButtonCallback(InputCallbackMouseButtonFn callback, MouseButton button, MouseAction action)
{
    InputCallbackMouseInfo info;
    info.callback = callback;
    info.button = button;
    info.action = action;
    info.token = GenerateToken();
    m_MouseCallbacks.push_back(info);
    return info.token;
}

InputCallbackToken InputSystem::AddMouseWheelCallback(InputCallbackMouseWheelFn callback)
{
    InputCallbackMouseWheelInfo info;
    info.callback = callback;
    info.token = GenerateToken();
    m_MouseWheelCallbacks.push_back(info);
    return info.token;
}

void InputSystem::RemoveKeyboardCallback( InputCallbackToken token )
{
    m_KeyboardCallbacks.remove_if( [ token ]( const InputCallbackKeyboardInfo& callbackInfo ) { return token == callbackInfo.token; } );
}

void InputSystem::RemoveMouseButtonCallback( InputCallbackToken token )
{
    m_MouseCallbacks.remove_if( [ token ]( const InputCallbackMouseInfo& callbackInfo ) { return token == callbackInfo.token; } );
}

void InputSystem::RemoveMouseWheelCallback( InputCallbackToken token )
{
    m_MouseWheelCallbacks.remove_if( [ token ]( const InputCallbackMouseWheelInfo& callbackInfo ) { return token == callbackInfo.token; } );
}

} // namespace WingsOfSteel::Pandora