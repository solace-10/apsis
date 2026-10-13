#pragma once

#if defined(TARGET_PLATFORM_NATIVE)

#include "input/private/input_system_impl.hpp"

#include <optional>

#include <glm/vec2.hpp>

namespace WingsOfSteel::Private
{

class InputSystemNative : public InputSystemImpl
{
public:
    InputSystemNative(InputSystem& parent);
    ~InputSystemNative() override;

    void Initialize() override;
    void Update() override;
    bool IsCursorLocked() const override;

    
    void HandleLockedMousePositionEvent(const glm::vec2& virtualPosition);
    void HandleUnlockedMousePositionEvent(const glm::vec2& position);

private:
    glm::vec2 m_CursorPosition{ 0.0f };
    std::optional<glm::vec2> m_PreviousVirtualCursorPosition;
};

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_NATIVE
