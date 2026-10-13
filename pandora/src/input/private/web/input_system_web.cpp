#if defined(TARGET_PLATFORM_WEB)

#include "input/private/web/input_system_web.hpp"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <emscripten/html5.h>

#include "input/input_system.hpp"
#include "pandora.hpp"
#include "render/window.hpp"

namespace WingsOfSteel::Private
{

InputSystemWeb* sInputSystemWeb = nullptr;

InputSystemWeb::InputSystemWeb(InputSystem& parent)
    : InputSystemImpl(parent)
{
}

InputSystemWeb::~InputSystemWeb()
{
    sInputSystemWeb = nullptr;
    
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();
    glfwSetCursorPosCallback(pWindow, nullptr);
    glfwSetScrollCallback(pWindow, nullptr);
    glfwSetCursorEnterCallback(pWindow, nullptr);
}

void InputSystemWeb::Initialize()
{
    sInputSystemWeb = this;
    
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();

    glfwSetKeyCallback(pWindow, [](GLFWwindow* pWindow, int key, int scancode, int action, int mods) {
        GetInputSystem()->HandleKeyboardEvent(key, scancode, action, mods);
    });

    glfwSetMouseButtonCallback(pWindow, [](GLFWwindow* pWindow, int button, int action, int mods) {
        GetInputSystem()->HandleMouseButtonEvent(button, action, mods);
    });

    glfwSetCursorPosCallback(pWindow, [](GLFWwindow* pWindow, double xPos, double yPos) {
        if (GetInputSystem()->GetCursorMode() == CursorMode::Locked)
        {
            // When the cursor is locked, the browser switches to sending deltas rather than positions.
            // See the Pointer Lock API: https://developer.mozilla.org/en-US/docs/Web/API/Pointer_Lock_API
            // Additionally, these deltas are just integers, which makes the cursor move oddly when doing small adjustments.
            // A better alternative might be to use the newer "pointerrawupdate" event which can be added to the canvas.
            sInputSystemWeb->HandleLockedMousePositionEvent(glm::vec2(xPos, yPos));
        }
        else
        {
            sInputSystemWeb->HandleUnlockedMousePositionEvent(glm::vec2(xPos, yPos));
        }
    });

    glfwSetScrollCallback(pWindow, [](GLFWwindow* pWindow, double xOffset, double yOffset) {
        GetInputSystem()->HandleMouseWheelEvent(xOffset, yOffset);
    });

    glfwSetCursorEnterCallback(pWindow, [](GLFWwindow* pWindow, int entered) {
        GetInputSystem()->HandleCursorEnterEvent(entered == GLFW_TRUE);
    });
}

void InputSystemWeb::Update()
{
}

bool InputSystemWeb::IsCursorLocked() const
{
    if (GetInputSystem()->GetCursorMode() != CursorMode::Locked)
    {
        return false;
    }
    
    EmscriptenPointerlockChangeEvent pointerLockStatus;
    if (emscripten_get_pointerlock_status(&pointerLockStatus) == EMSCRIPTEN_RESULT_SUCCESS)
    {
        return pointerLockStatus.isActive;
    }
    else
    {
        return false;
    }
}

void InputSystemWeb::HandleLockedMousePositionEvent(const glm::vec2& mouseDelta)
{
    m_CursorPosition += mouseDelta;
    m_CursorPosition.x = glm::clamp(m_CursorPosition.x, 0.0f, static_cast<float>(GetWindow()->GetWidth()));
    m_CursorPosition.y = glm::clamp(m_CursorPosition.y, 0.0f, static_cast<float>(GetWindow()->GetHeight()));
    GetInputSystem()->HandleMousePositionEvent(m_CursorPosition);
}

void InputSystemWeb::HandleUnlockedMousePositionEvent(const glm::vec2& mousePosition)
{
    GetInputSystem()->HandleMousePositionEvent(mousePosition);
}

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_WEB
