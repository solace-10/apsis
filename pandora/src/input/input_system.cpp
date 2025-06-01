#include <GLFW/glfw3.h>
#include <imgui.h>

#include "core/log.hpp"
#include "input/input_system.hpp"
#include "pandora.hpp"
#include "render/window.hpp"

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
    glfwSetMouseButtonCallback(pWindow, nullptr);
    glfwSetCursorPosCallback(pWindow, nullptr);
    glfwSetScrollCallback(pWindow, nullptr);
    glfwSetCursorEnterCallback(pWindow, nullptr);
}

void InputSystem::Initialize()
{
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();

    glfwSetKeyCallback(pWindow, [](GLFWwindow* pWindow, int key, int scancode, int action, int mods) {
        GetInputSystem()->HandleKeyboardEvent(key, scancode, action, mods);
    });

    glfwSetMouseButtonCallback(pWindow, [](GLFWwindow* pWindow, int button, int action, int mods) {
        GetInputSystem()->HandleMouseButtonEvent(button, action, mods);
    });

    glfwSetCursorPosCallback(pWindow, [](GLFWwindow* pWindow, double xPos, double yPos) {
        GetInputSystem()->HandleMousePositionEvent(xPos, yPos);
    });

    glfwSetScrollCallback(pWindow, [](GLFWwindow* pWindow, double xOffset, double yOffset) {
        GetInputSystem()->HandleMouseWheelEvent(xOffset, yOffset);
    });

    glfwSetCursorEnterCallback(pWindow, [](GLFWwindow* pWindow, int entered) {
        GetInputSystem()->HandleCursorEnterEvent(entered == GLFW_TRUE);
    });
}

void InputSystem::HandleMouseWheelEvent(double xOffset, double yOffset)
{
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    const glm::vec2 offset(static_cast<float>(xOffset), static_cast<float>(yOffset));
    for (auto& callbackInfo : m_MouseWheelCallbacks)
    {
        callbackInfo.callback(offset);
    }
}

void InputSystem::HandleKeyboardEvent(int key, int scancode, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return;
    }

    for (auto& callbackInfo : m_KeyboardCallbacks)
    {
        if (callbackInfo.key == key)
        {
            if ((callbackInfo.action == KeyAction::Pressed && action == GLFW_PRESS) || (callbackInfo.action == KeyAction::Released && action == GLFW_RELEASE) || (callbackInfo.action == KeyAction::Held && action == GLFW_REPEAT))
            {
                callbackInfo.callback();
            }
        }
    }
}

void InputSystem::HandleMouseButtonEvent(int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    for (auto& callbackInfo : m_MouseCallbacks)
    {
        if (static_cast<int>(callbackInfo.button) == button)
        {
            if ((callbackInfo.action == MouseAction::Pressed && action == GLFW_PRESS) || (callbackInfo.action == MouseAction::Released && action == GLFW_RELEASE))
            {
                callbackInfo.callback();
            }
        }
    }
}

void InputSystem::HandleMousePositionEvent(double xPos, double yPos)
{
    const glm::vec2 mousePosition(static_cast<float>(xPos), static_cast<float>(yPos));
    glm::vec2 mouseDelta(0.0f);
    if (m_PreviousMousePosition)
    {
        mouseDelta = mousePosition - m_PreviousMousePosition.value();
    }
    m_PreviousMousePosition = mousePosition;

    for (auto& callbackInfo : m_MousePositionCallbacks)
    {
        callbackInfo.callback(mousePosition, mouseDelta);
    }
}

void InputSystem::HandleCursorEnterEvent(bool entered)
{
    if (entered)
    {
        m_PreviousMousePosition.reset();
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

InputCallbackToken InputSystem::AddMousePositionCallback(InputCallbackMousePositionFn callback)
{
    InputCallbackMousePositionInfo info;
    info.callback = callback;
    info.token = GenerateToken();
    m_MousePositionCallbacks.push_back(info);
    return info.token;
}

void InputSystem::RemoveKeyboardCallback(InputCallbackToken token)
{
    m_KeyboardCallbacks.remove_if([token](const InputCallbackKeyboardInfo& callbackInfo) { return token == callbackInfo.token; });
}

void InputSystem::RemoveMouseButtonCallback(InputCallbackToken token)
{
    m_MouseCallbacks.remove_if([token](const InputCallbackMouseInfo& callbackInfo) { return token == callbackInfo.token; });
}

void InputSystem::RemoveMouseWheelCallback(InputCallbackToken token)
{
    m_MouseWheelCallbacks.remove_if([token](const InputCallbackMouseWheelInfo& callbackInfo) { return token == callbackInfo.token; });
}

void InputSystem::RemoveMousePositionCallback(InputCallbackToken token)
{
    m_MousePositionCallbacks.remove_if([token](const InputCallbackMousePositionInfo& callbackInfo) { return token == callbackInfo.token; });
}

} // namespace WingsOfSteel::Pandora