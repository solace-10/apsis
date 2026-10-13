#if defined(TARGET_PLATFORM_NATIVE)

#include "input/private/native/input_system_native.hpp"

#include <glm/common.hpp>
#include <GLFW/glfw3.h>

#include "input/input_system.hpp"
#include "pandora.hpp"
#include "render/window.hpp"

namespace WingsOfSteel::Private
{

InputSystemNative* sInputSystemNative = nullptr;

InputSystemNative::InputSystemNative(InputSystem& parent)
    : InputSystemImpl(parent)
{
}

InputSystemNative::~InputSystemNative()
{
    sInputSystemNative = nullptr;
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();
    glfwSetKeyCallback(pWindow, nullptr);
    glfwSetMouseButtonCallback(pWindow, nullptr);
    glfwSetCursorPosCallback(pWindow, nullptr);
    glfwSetScrollCallback(pWindow, nullptr);
    glfwSetCursorEnterCallback(pWindow, nullptr);
}

void InputSystemNative::Initialize()
{
    sInputSystemNative = this;
    GLFWwindow* pWindow = GetWindow()->GetRawWindow();

    glfwSetKeyCallback(pWindow, [](GLFWwindow* pWindow, int key, int scancode, int action, int mods) {
        GetInputSystem()->HandleKeyboardEvent(key, scancode, action, mods);
    });

    glfwSetMouseButtonCallback(pWindow, [](GLFWwindow* pWindow, int button, int action, int mods) {
        GetInputSystem()->HandleMouseButtonEvent(button, action, mods);
    });

    glfwSetCursorPosCallback(pWindow, [](GLFWwindow* pWindow, double xPos, double yPos) {
        if (sInputSystemNative->IsCursorLocked())
        {
            sInputSystemNative->HandleLockedMousePositionEvent(glm::vec2(xPos, yPos));
        }
        else
        {            
            sInputSystemNative->HandleUnlockedMousePositionEvent(glm::vec2(xPos, yPos));
        }
    });

    glfwSetScrollCallback(pWindow, [](GLFWwindow* pWindow, double xOffset, double yOffset) {
        GetInputSystem()->HandleMouseWheelEvent(xOffset, yOffset);
    });

    glfwSetCursorEnterCallback(pWindow, [](GLFWwindow* pWindow, int entered) {
        // Note: GLFW only triggers this event if IsCursorLocked() is false.
        GetInputSystem()->HandleCursorEnterEvent(entered == GLFW_TRUE);
    });

    // If possible, get the positions based on the raw motion, which is not affected by OS-level
    // settings regarding mouse acceleration.
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

void InputSystemNative::Update()
{
}

bool InputSystemNative::IsCursorLocked() const
{
    return GetInputSystem()->GetCursorMode() == CursorMode::Locked &&  glfwGetWindowAttrib(GetWindow()->GetRawWindow(), GLFW_FOCUSED);
}

// When locked, we receive mouse positions as if we were in a window of infinite size.
// We map this to the window size, calculating the new cursor's position from the virtual cursor's deltas. 
void InputSystemNative::HandleLockedMousePositionEvent(const glm::vec2& virtualPosition)
{
    if (!m_PreviousVirtualCursorPosition)
    {
        m_PreviousVirtualCursorPosition = virtualPosition; 
        m_CursorPosition.x = static_cast<float>(GetWindow()->GetWidth() / 2.0f);
        m_CursorPosition.y = static_cast<float>(GetWindow()->GetHeight() / 2.0f);
    }

    const glm::vec2 virtualPositionDelta = virtualPosition - m_PreviousVirtualCursorPosition.value();
    m_PreviousVirtualCursorPosition = virtualPosition;
    m_CursorPosition += virtualPositionDelta;
    m_CursorPosition.x = glm::clamp(m_CursorPosition.x, 0.0f, static_cast<float>(GetWindow()->GetWidth()));
    m_CursorPosition.y = glm::clamp(m_CursorPosition.y, 0.0f, static_cast<float>(GetWindow()->GetHeight()));
    
    GetInputSystem()->HandleMousePositionEvent(m_CursorPosition);
}

void InputSystemNative::HandleUnlockedMousePositionEvent(const glm::vec2& position)
{
    GetInputSystem()->HandleMousePositionEvent(position);
}

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_NATIVE
