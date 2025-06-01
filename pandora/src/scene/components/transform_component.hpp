#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace WingsOfSteel::Pandora
{

struct TransformComponent
{
    glm::mat4 transform;
};

} // namespace WingsOfSteel::Pandora