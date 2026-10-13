#pragma once

#include <glm/vec3.hpp>
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel
{

class DirectionalLightComponent : public IComponent
{
public:
    DirectionalLightComponent() = default;
    
    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    void SetAngle(float angleInDegrees);
    float GetAngle() const { return m_Angle; }
    void SetPitch(float pitchInDegrees);
    float GetPitch() const { return m_Pitch; }

    // The direction the light comes *from*, as used in shaders: NdotL = dot(normal, direction).
    // Angle and pitch are a way of authoring this by hand; a caller that already has the vector -
    // anything driven by a real ephemeris, say - can set it directly and skip the round trip.
    void SetDirection(const glm::vec3& direction);
    const glm::vec3& GetNormalizedDirection() const { return m_Direction; }

    const glm::vec3& GetColor() const { return m_Color; }
    void SetColor(float r, float g, float b) { SetColor(glm::vec3(r, g, b)); }
    void SetColor(const glm::vec3& color) { m_Color = color; }

    float GetIntensity() const { return m_Intensity; }
    void SetIntensity(float intensity) { m_Intensity = intensity; }

private:
    void CalculateNormalizedDirection();
    
    glm::vec3 m_Direction{ 0.0f, -1.0f, 0.0f };
    glm::vec3 m_Color{ 1.0f, 1.0f, 1.0f };
    float m_Intensity{ 1.0f };
    float m_Angle{ 0.0f };
    float m_Pitch{ 0.0f };
};

REGISTER_COMPONENT(DirectionalLightComponent, "directional_light")

} // namespace WingsOfSteel
