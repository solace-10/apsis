#pragma once

#include <chrono>
#include <optional>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class OrbitalElementsComponent : public IComponent
{
public:
    OrbitalElementsComponent() = default;
    ~OrbitalElementsComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    std::chrono::system_clock::time_point m_Epoch;
    float m_MeanMotion{ 0.0f };            // rev/day
    float m_Eccentricity{ 0.0f };
    float m_Inclination{ 0.0f };           // degrees
    float m_RightAscensionOfAscendingNode{ 0.0f }; // degrees
    float m_ArgumentOfPericenter{ 0.0f };  // degrees
    float m_MeanAnomaly{ 0.0f };           // degrees

    // Optional fields for future SGP4 implementation
    std::optional<float> m_MeanMotionFirstDerivative;
    std::optional<float> m_MeanMotionSecondDerivative;
};

REGISTER_COMPONENT(OrbitalElementsComponent, "orbital_elements")

} // namespace WingsOfSteel
