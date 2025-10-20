#pragma once

#include <unordered_map>

#include <glm/vec3.hpp>

#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel
{

class Threat
{
public:
    glm::vec3 position{ 0.0f };
    float timeSinceSpawned{ 0.0f };
    float timeSinceLastSeen{ 0.0f };
    uint32_t lastUpdate{ 0 };
};

class ThreatIndicatorSystem : public System
{
public:
    ThreatIndicatorSystem() = default;
    ~ThreatIndicatorSystem() = default;

    void Initialize(Scene* pScene) override {};

    void Update(float delta) override;

private:
    using ThreatMap = std::unordered_map<EntityHandle, Threat>;
    ThreatMap m_Threats;
    uint32_t m_CurrentUpdate;
};

} // namespace WingsOfSteel
