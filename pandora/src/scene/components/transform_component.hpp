#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace WingsOfSteel::Pandora
{

struct TransformComponent
{
    glm::mat4 transform;
};

}