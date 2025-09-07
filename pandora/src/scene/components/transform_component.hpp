#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel
{

class TransformComponent : public IComponent
{
public:
    TransformComponent() = default;
    const glm::vec3 GetTranslation() const { return transform[3]; }
    const glm::vec3 GetForward() const { return glm::normalize(transform[2]); }
    const glm::vec3 GetRight() const { return glm::normalize(transform[0]); }
    const glm::vec3 GetUp() const { return glm::normalize(transform[1]); }

    glm::mat4 transform{ 1.0f };

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["transform"] = SerializeMat4(transform);
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        transform = DeserializeMat4(json, "transform");
    }
};

REGISTER_COMPONENT(TransformComponent, "transform")

} // namespace WingsOfSteel