#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <core/serialization.hpp>
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
        m_NoradId = static_cast<int32_t>(Json::DeserializeInteger(pContext, json, "norad_id"));
        m_Epoch = ParseEpoch(Json::DeserializeString(pContext, json, "epoch"));
        m_MeanMotion = Json::DeserializeFloat(pContext, json, "mean_motion");
        m_Eccentricity = Json::DeserializeFloat(pContext, json, "eccentricity");
        m_Inclination = Json::DeserializeFloat(pContext, json, "inclination");
        m_RightAscensionOfAscendingNode = Json::DeserializeFloat(pContext, json, "raan");
        m_ArgumentOfPericenter = Json::DeserializeFloat(pContext, json, "arg_of_pericenter");
        m_MeanAnomaly = Json::DeserializeFloat(pContext, json, "mean_anomaly");

        // Not implemented yet.
        //m_MeanMotionFirstDerivative = Json::DeserializeFloat(pContext, json, "mean_motion_dot");
        //m_MeanMotionSecondDerivative = Json::DeserializeFloat(pContext, json, "mean_motion_ddot");
    }

    int32_t GetNoradId() const { return m_NoradId; }
    const std::chrono::system_clock::time_point& GetEpoch() const { return m_Epoch; }
    float GetMeanMotion() const { return m_MeanMotion; }
    float GetEccentricity() const { return m_Eccentricity; }
    float GetInclination() const { return m_Inclination; }
    float GetRightAscensionOfAscendingNode() const { return m_RightAscensionOfAscendingNode; }
    float GetArgumentOfPericenter() const { return m_ArgumentOfPericenter; }
    float GetMeanAnomaly() const { return m_MeanAnomaly; }
    float GetMeanMotionFirstDerivative() const { return m_MeanMotionFirstDerivative; }
    float GetMeanMotionSecondDerivative() const { return m_MeanMotionSecondDerivative; }

private:
    static std::chrono::system_clock::time_point ParseEpoch(const std::string& epochStr)
    {
        std::tm tm = {};
        std::istringstream ss(epochStr);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

#ifdef _WIN32
        std::time_t time = _mkgmtime(&tm);
#else
        std::time_t time = timegm(&tm);
#endif

        auto tp = std::chrono::system_clock::from_time_t(time);

        if (ss.peek() == '.')
        {
            ss.get();
            std::string fractional;
            ss >> fractional;
            fractional.resize(6, '0');
            int microseconds = std::stoi(fractional);
            tp += std::chrono::microseconds(microseconds);
        }

        return tp;
    }

    int32_t m_NoradId{ 0 };
    std::chrono::system_clock::time_point m_Epoch;
    float m_MeanMotion{ 0.0f };            // rev/day
    float m_Eccentricity{ 0.0f };
    float m_Inclination{ 0.0f };           // degrees
    float m_RightAscensionOfAscendingNode{ 0.0f }; // degrees
    float m_ArgumentOfPericenter{ 0.0f };  // degrees
    float m_MeanAnomaly{ 0.0f };           // degrees
    float m_MeanMotionFirstDerivative{ 0.0f };
    float m_MeanMotionSecondDerivative{ 0.0f };
};

REGISTER_COMPONENT(OrbitalElementsComponent, "orbital_elements")

} // namespace WingsOfSteel
