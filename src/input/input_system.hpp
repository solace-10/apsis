#pragma once

#include <array>
#include <functional>
#include <list>
#include <memory>
#include <optional>

#include <glm/vec2.hpp>

namespace WingsOfSteel
{

namespace Private
{
class InputSystemImpl;
}

enum class CursorMode
{
    Normal, // The cursor can enter and leave the window at will.
    Locked  // The cursor is locked inside the window and cannot leave (other than by another window getting focus on native, or pressing escape on Web)
};

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
    void Update();

    InputCallbackToken AddKeyboardCallback(InputCallbackKeyboardFn callback, int key, KeyAction action);
    InputCallbackToken AddMouseButtonCallback(InputCallbackMouseButtonFn callback, MouseButton button, MouseAction action);
    InputCallbackToken AddMouseWheelCallback(InputCallbackMouseWheelFn callback);
    InputCallbackToken AddMousePositionCallback(InputCallbackMousePositionFn callback);

    void RemoveKeyboardCallback(InputCallbackToken token);
    void RemoveMouseButtonCallback(InputCallbackToken token);
    void RemoveMouseWheelCallback(InputCallbackToken token);
    void RemoveMousePositionCallback(InputCallbackToken token);

    void HandleKeyboardEvent(int key, int scancode, int action, int mods);
    void HandleMouseButtonEvent(int button, int action, int mods);
    void HandleMousePositionEvent(const glm::vec2& position);
    void HandleMouseWheelEvent(double xOffset, double yOffset);
    void HandleCursorEnterEvent(bool entered);

    CursorMode GetCursorMode() const { return m_CursorMode; }
    void SetCursorMode(CursorMode cursorMode);
    const std::optional<glm::vec2> GetCursorPosition() const { return m_CursorPosition; }

    static InputCallbackToken sInvalidInputCallbackToken;

private:
    friend class Private::InputSystemImpl;
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
    bool IsCursorLocked() const;

    CallbackKeyboardList m_KeyboardCallbacks;
    CallbackMouseList m_MouseCallbacks;
    CallbackMouseWheelList m_MouseWheelCallbacks;
    CallbackMousePositionList m_MousePositionCallbacks;

    static InputCallbackToken m_sToken;

    std::optional<glm::vec2> m_PreviousCursorPosition;
    std::optional<glm::vec2> m_CursorPosition;
    bool m_IsCursorVisible{ false };
    CursorMode m_CursorMode{ CursorMode::Normal };

    std::unique_ptr<Private::InputSystemImpl> m_pImpl;
};

//////////////////////////////////////////////////////////////////////////
// Inline implementations
//////////////////////////////////////////////////////////////////////////

inline InputCallbackToken InputSystem::GenerateToken()
{
    return m_sToken++;
}

} // namespace WingsOfSteel
