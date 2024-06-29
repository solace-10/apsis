#include <glm/gtc/matrix_transform.hpp>

#include "scene/camera.hpp"

namespace WingsOfSteel::Pandora
{

Camera::Camera()
{

}
    
Camera::~Camera()
{

}

void Camera::LookAt(const glm::vec3& cameraPosition, const glm::vec3& targetPosition, const glm::vec3& up)
{
    SetTransform(glm::lookAt(cameraPosition, targetPosition, up));
}

} // namespace WingsOfSteel::Pandora