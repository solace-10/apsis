#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace WingsOfSteel::Pandora
{

class TransformComponent
{
public:
    TransformComponent() = default;
    const glm::vec3 GetTranslation() const { return transform[3]; }
    const glm::vec3 GetForward() const { return transform[2]; }
    const glm::vec3 GetRight() const { return transform[0]; }
    const glm::vec3 GetUp() const { return transform[1]; }

    glm::mat4 transform;
};

} // namespace WingsOfSteel::Pandora