#pragma once

#include <glm/vec2.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class MechNavigationComponent
{
public:
    MechNavigationComponent() = default;
    ~MechNavigationComponent() = default;

    const glm::vec2& GetThrust() const { return m_Thrust; }
    void SetThrust(const glm::vec2& thrust) { m_Thrust = thrust; }

private:
    glm::vec2 m_Thrust{0.0f};
};

} // namespace WingsOfSteel::TheBrightestStar