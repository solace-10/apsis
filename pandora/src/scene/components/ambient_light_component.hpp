#pragma once

#include <glm/vec3.hpp>
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel
{

class AmbientLightComponent : public IComponent
{
public:
    AmbientLightComponent() = default;

    
    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    const glm::vec3& GetColor() const { return m_Color; }
    void SetColor(float r, float g, float b) { SetColor(glm::vec3(r, g, b)); }
    void SetColor(const glm::vec3& color) { m_Color = color; }

    float GetIntensity() const { return m_Intensity; }
    void SetIntensity(float intensity) { m_Intensity = intensity; }

private:
    glm::vec3 m_Color{ 1.0f, 1.0f, 1.0f };
    float m_Intensity{ 1.0f };
};

REGISTER_COMPONENT(AmbientLightComponent, "ambient_light")

} // namespace WingsOfSteel
