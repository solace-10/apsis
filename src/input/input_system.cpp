#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#if defined(TARGET_PLATFORM_NATIVE)
#include "input/private/native/input_system_native.hpp"
#elif defined(TARGET_PLATFORM_WEB)
#include "input/private/web/input_system_web.hpp"
#endif

#include "core/log.hpp"
#include "imgui/imgui.hpp"
#include "input/input_system.hpp"
#include "pandora.hpp"
#include "render/window.hpp"

namespace WingsOfSteel
{

InputCallbackToken InputSystem::m_sToken = 1u;
InputCallbackToken InputSystem::sInvalidInputCallbackToken = 0u;

InputSystem::InputSystem()
{
}

InputSystem::~InputSystem()
{
}

void InputSystem::Initialize()
{
#if defined(TARGET_PLATFORM_NATIVE)
    m_pImpl = std::make_unique<Private::InputSystemNative>(*this);
#elif defined(TARGET_PLATFORM_WEB)
    m_pImpl = std::make_unique<Private::InputSystemWeb>(*this);
#endif

    m_pImpl->Initialize();
    SetCursorMode(CursorMode::Normal);
}

void InputSystem::Update()
{
    m_pImpl->Update();

    if (IsCursorLocked() && m_CursorPosition.has_value())
    {
        ImGui::GetForegroundDrawList()->AddCircleFilled(m_CursorPosition.value(), 4.0f, IM_COL32(255, 0, 0, 255));
    }
}

void InputSystem::SetCursorMode(CursorMode cursorMode)
{
    m_CursorMode = cursorMode;
    
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();
    if (cursorMode == CursorMode::Normal)
    {
        glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else if (cursorMode == CursorMode::Locked)
    {
        // GLFW_CURSOR_DISABLED locks the cursor to the window, hides it and all cursor positions become
        // virtual (unbounded by the window or monitor's resolution).
        // This lets us have an in-game cursor which is consistent between Web and Native, and for the
        // Web client it works in conjunction with `requestPointerLock` in the game's shell file.
        glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void InputSystem::HandleMouseWheelEvent(double xOffset, double yOffset)
{
#if defined(TARGET_PLATFORM_NATIVE)
    // There is an issue with the current version of ImGUI where web builds can spuriously set WantCaptureMouse/WantCaptureKeyboard, causing events to be lost.
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }
#endif

    const glm::vec2 offset(static_cast<float>(xOffset), static_cast<float>(yOffset));
    for (auto& callbackInfo : m_MouseWheelCallbacks)
    {
        callbackInfo.callback(offset);
    }
}

void InputSystem::HandleKeyboardEvent(int key, int scancode, int action, int mods)
{
#if defined(TARGET_PLATFORM_NATIVE)
    // There is an issue with the current version of ImGUI where web builds can spuriously set WantCaptureMouse/WantCaptureKeyboard, causing events to be lost.
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return;
    }
#endif

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
#if defined(TARGET_PLATFORM_NATIVE)
    // There is an issue with the current version of ImGUI where web builds can spuriously set WantCaptureMouse/WantCaptureKeyboard, causing events to be lost.
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }
#endif

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

// The expected mouse position should be between (0,0) and (windowWidth,windowHeight).
// Mouse deltas are calculated from the previous position if available.
// The mouse delta will be (0,0) if the mouse has just returned to the window.
void InputSystem::HandleMousePositionEvent(const glm::vec2& position)
{
    m_PreviousCursorPosition = m_CursorPosition;

    m_CursorPosition = glm::vec2(
        glm::clamp(position.x, 0.0f, static_cast<float>(GetWindow()->GetWidth())),
        glm::clamp(position.y, 0.0f, static_cast<float>(GetWindow()->GetHeight())));

    glm::vec2 cursorDelta(0.0f, 0.0f);
    if (m_PreviousCursorPosition.has_value() && m_CursorPosition.has_value())
    {
        cursorDelta = m_CursorPosition.value() - m_PreviousCursorPosition.value();
    }

    if (m_CursorPosition.has_value())
    {
        const glm::vec2& cursorPosition = m_CursorPosition.value();

        for (auto& callbackInfo : m_MousePositionCallbacks)
        {
            callbackInfo.callback(cursorPosition, cursorDelta);
        }
    }
}

void InputSystem::HandleCursorEnterEvent(bool entered)
{
    if (entered)
    {
        m_PreviousCursorPosition.reset();
    }
    else
    {
        m_CursorPosition.reset();
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

bool InputSystem::IsCursorLocked() const
{
    return m_pImpl->IsCursorLocked();
}

} // namespace WingsOfSteel
