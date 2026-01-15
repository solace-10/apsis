#pragma once

#include <optional>
#include <string>

#include <resources/resource_data_store.hpp>

namespace WingsOfSteel
{

class SpaceObject
{
public:
    SpaceObject();
    ~SpaceObject();

    bool DeserializeOMM(const Json::Data& data);

    const std::string& GetObjectName() const { return m_ObjectName; }
    const std::string& GetObjectId() const { return m_ObjectId; }
    const std::string& GetEpoch() const { return m_Epoch; }
    float GetMeanMotion() const { return m_MeanMotion; }
    float GetEccentricity() const { return m_Eccentricity; }
    float GetInclination() const { return m_Inclination; }
    float GetRightAscensionOfAscendingNode() const { return m_RightAscensionOfAscendingNode; }
    float GetArgumentOfPericenter() const { return m_ArgumentOfPericenter; }
    float GetMeanAnomaly() const { return m_MeanAnomaly; }
    uint32_t GetNoradCatalogueId() const { return m_NoradCatalogueId; }

private:
    std::string m_ObjectName{ "UNKNOWN" };
    std::string m_ObjectId{ "0" };
    std::string m_Epoch;
    float m_MeanMotion{ 0.0f };
    float m_Eccentricity{ 0.0f };
    float m_Inclination{ 0.0f };
    float m_RightAscensionOfAscendingNode{ 0.0f };
    float m_ArgumentOfPericenter{ 0.0f };
    float m_MeanAnomaly{ 0.0f };
    uint32_t m_NoradCatalogueId{ 0 };

    std::optional<uint32_t> m_ElementSetNumber{ 0 };
    std::optional<uint32_t> m_RevolutionsAtEpoch{ 0 };
    std::optional<float> m_MeanMotionFirstDerivative{ 0.0f };
    std::optional<float> m_MeanMotionSecondDerivative{ 0.0f };
};

}
