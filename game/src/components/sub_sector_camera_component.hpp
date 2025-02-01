#pragma once

#include <glm/vec3.hpp>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::Pandora
{
    DECLARE_SMART_PTR(Entity);
}

namespace WingsOfSteel::TheBrightestStar
{

class SubSectorCameraComponent
{
public:
    SubSectorCameraComponent() {}
    ~SubSectorCameraComponent() {}

    glm::vec3 position{0.0f};
    glm::vec3 maximumDrift{0.0f};
    float driftTimer{0.0f};
    glm::vec3 target{0.0f};
    glm::vec3 velocity{0.0f};
    Pandora::EntityWeakPtr anchorEntity;
    bool debugDraw{false};
};

} // namespace WingsOfSteel::TheBrightestStar