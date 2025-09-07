#pragma once

#include <array>
#include <functional>
#include <list>
#include <optional>

#include <glm/vec2.hpp>

namespace WingsOfSteel
{
enum class MouseButton
{
    Left,
    Right,
    Middle
};

enum class KeyAction
{
    Pressed,
    Released,
    Held
};

enum class MouseAction
{
    Pressed,
    Released
};

using InputCallbackToken = unsigned int;
using InputCallbackKeyboardFn = std::function<void()>;
using InputCallbackMouseButtonFn = std::function<void()>;
using InputCallbackMouseWheelFn = std::function<void(const glm::vec2&)>;
using InputCallbackMousePositionFn = std::function<void(const glm::vec2&, const glm::vec2&)>;

class InputSystem
{
public:
    InputSystem();
    ~InputSystem();
    void Initialize();

    InputCallbackToken AddKeyboardCallback(InputCallbackKeyboardFn callback, int key, KeyAction action);
    InputCallbackToken AddMouseButtonCallback(InputCallbackMouseButtonFn callback, MouseButton button, MouseAction action);
    InputCallbackToken AddMouseWheelCallback(InputCallbackMouseWheelFn callback);
    InputCallbackToken AddMousePositionCallback(InputCallbackMousePositionFn callback);

    void RemoveKeyboardCallback(InputCallbackToken token);
    void RemoveMouseButtonCallback(InputCallbackToken token);
    void RemoveMouseWheelCallback(InputCallbackToken token);
    void RemoveMousePositionCallback(InputCallbackToken token);

    static InputCallbackToken sInvalidInputCallbackToken;

private:
    struct InputCallbackKeyboardInfo
    {
        InputCallbackToken token;
        InputCallbackKeyboardFn callback;
        int key;
        KeyAction action;
    };

    struct InputCallbackMouseInfo
    {
        InputCallbackToken token;
        InputCallbackMouseButtonFn callback;
        MouseButton button;
        MouseAction action;
    };

    struct InputCallbackMouseWheelInfo
    {
        InputCallbackToken token;
        InputCallbackMouseWheelFn callback;
    };

    struct InputCallbackMousePositionInfo
    {
        InputCallbackToken token;
        InputCallbackMousePositionFn callback;
    };

    using CallbackKeyboardList = std::list<InputCallbackKeyboardInfo>;
    using CallbackMouseList = std::list<InputCallbackMouseInfo>;
    using CallbackMouseWheelList = std::list<InputCallbackMouseWheelInfo>;
    using CallbackMousePositionList = std::list<InputCallbackMousePositionInfo>;

    InputCallbackToken GenerateToken();

    void HandleKeyboardEvent(int key, int scancode, int action, int mods);
    void HandleMouseButtonEvent(int button, int action, int mods);
    void HandleMousePositionEvent(double xPos, double yPos);
    void HandleMouseWheelEvent(double xOffset, double yOffset);
    void HandleCursorEnterEvent(bool entered);

    CallbackKeyboardList m_KeyboardCallbacks;
    CallbackMouseList m_MouseCallbacks;
    CallbackMouseWheelList m_MouseWheelCallbacks;
    CallbackMousePositionList m_MousePositionCallbacks;

    static InputCallbackToken m_sToken;

    std::optional<glm::vec2> m_PreviousMousePosition;
};

//////////////////////////////////////////////////////////////////////////
// Inline implementations
//////////////////////////////////////////////////////////////////////////

inline InputCallbackToken InputSystem::GenerateToken()
{
    return m_sToken++;
}

} // namespace WingsOfSteel
