#pragma once

#include <array>
#include <functional>

#include <glm/vec2.hpp>

namespace WingsOfSteel::Pandora
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

typedef unsigned int InputCallbackToken;
typedef std::function<void()> InputCallbackKeyboardFn;
typedef std::function<void()> InputCallbackMouseButtonFn;
typedef std::function<void(float x, float y)> InputCallbackMouseWheelFn;

class InputSystem
{
public:
    InputSystem();
    ~InputSystem();
    void Initialize();

    const glm::vec2& GetMousePosition() const;

    InputCallbackToken AddKeyboardCallback(InputCallbackKeyboardFn callback, int key, KeyAction action);
    InputCallbackToken AddMouseButtonCallback(InputCallbackMouseButtonFn callback, MouseButton button, MouseAction action);
    InputCallbackToken AddMouseWheelCallback(InputCallbackMouseWheelFn callback);

    void RemoveKeyboardCallback(InputCallbackToken token);
    void RemoveMouseButtonCallback(InputCallbackToken token);
    void RemoveMouseWheelCallback(InputCallbackToken token);

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

    typedef std::list<InputCallbackKeyboardInfo> CallbackKeyboardList;
    typedef std::list<InputCallbackMouseInfo> CallbackMouseList;
    typedef std::list<InputCallbackMouseWheelInfo> CallbackMouseWheelList;

    InputCallbackToken GenerateToken();

    void HandleKeyboardEvent(int key, int scancode, int action, int mods);
    void HandleMouseButtonEvent(int button, int scancode, int action, int mods);
    void HandleMouseWheelEvent(double xOffset, double yOffset);

    CallbackKeyboardList m_KeyboardCallbacks;
    CallbackMouseList m_MouseCallbacks;
    CallbackMouseWheelList m_MouseWheelCallbacks;

    glm::vec2 m_MousePosition;
    static InputCallbackToken m_sToken;
};

//////////////////////////////////////////////////////////////////////////
// Inline implementations
//////////////////////////////////////////////////////////////////////////

inline InputCallbackToken InputSystem::GenerateToken()
{
    return m_sToken++;
}

inline const glm::vec2& InputSystem::GetMousePosition() const
{
    return m_MousePosition;
}

} // namespace WingsOfSteel::Pandora
