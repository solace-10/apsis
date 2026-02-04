#pragma once

#include <glm/vec3.hpp>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class OrbitalStateComponent : public IComponent
{
public:
    OrbitalStateComponent() = default;
    ~OrbitalStateComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    glm::dvec3 m_PositionECI{ 0.0 };  // km, Earth-Centered Inertial coordinates
    double m_SemiMajorAxis{ 0.0 };     // km
    double m_Altitude{ 0.0 };          // km above Earth's mean radius
    double m_Velocity{ 0.0 };          // km/s
    double m_Latitude{ 0.0 };          // degrees
    double m_Longitude{ 0.0 };         // degrees
};

REGISTER_COMPONENT(OrbitalStateComponent, "orbital_state")

} // namespace WingsOfSteel
