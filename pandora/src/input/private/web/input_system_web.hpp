#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <vector>

#include <glm/vec2.hpp>

#include "input/private/input_system_impl.hpp"

namespace WingsOfSteel::Private
{

class InputSystemWeb : public InputSystemImpl
{
public:
    InputSystemWeb(InputSystem& parent);
    ~InputSystemWeb() override;

    void Initialize() override;
    void Update() override;
    bool IsCursorLocked() const override;

    void HandleLockedMousePositionEvent(const glm::vec2& mouseDelta);
    void HandleUnlockedMousePositionEvent(const glm::vec2& mousePosition);

private:
    static const std::vector<int>& GetPolledKeys();

    glm::vec2 m_CursorPosition{ 0.0f };
};

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_WEB
