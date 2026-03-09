#pragma once

#include <optional>

#include <glm/vec2.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class SpaceObjectPickingSystem : public System
{
public:
    SpaceObjectPickingSystem() = default;
    ~SpaceObjectPickingSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

private:
    void PerformPick(const glm::vec2& screenPos);

    static constexpr float kPickRadiusPixels = 16.0f;

    InputCallbackToken m_LeftMouseButtonPressedToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_LeftMouseButtonReleasedToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_MousePositionToken{ InputSystem::sInvalidInputCallbackToken };
    std::optional<glm::vec2> m_PressPosition;
    glm::vec2 m_CurrentMousePosition{ 0.0f };
};

} // namespace WingsOfSteel
