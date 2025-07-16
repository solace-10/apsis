#pragma once

#include <glm/vec3.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class MechNavigationComponent
{
public:
    MechNavigationComponent() = default;
    ~MechNavigationComponent() = default;

    const glm::vec3& GetThrust() const { return m_Thrust; }
    void SetThrust(const glm::vec3& thrust) { m_Thrust = thrust; }

    const glm::vec3& GetAim() const { return m_Aim; }
    void SetAim(const glm::vec3& aim) { m_Aim = aim; }

private:
    glm::vec3 m_Thrust{0.0f};
    glm::vec3 m_Aim{0.0f};
};

} // namespace WingsOfSteel::TheBrightestStar