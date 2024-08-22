#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource_data_store.hpp>

#include "sector/signal/signal.hpp"
#include "sector/sector_info.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SectorGenerator);
class SectorGenerator
{
public:
    SectorGenerator();
    ~SectorGenerator();

    SectorInfoSharedPtr Create(uint32_t level);
    bool CanCreate(uint32_t level) const;

private:
    std::string GetPathForLevel(uint32_t level) const;
    void LoadGenerationParameters();

    std::vector<SignalSharedPtr> GenerateSignals(const Pandora::ResourceDataStoreSharedPtr& generationParameters) const;
    SignalSharedPtr GenerateSignal(const std::string& signalType, const nlohmann::json& generationParameters) const;
    SignalSharedPtr GenerateAsteroidFieldSignal(const nlohmann::json& generationParameters) const;
    glm::vec3 GenerateSignalPosition() const;
    SectorCivilizationLevel GenerateCivilizationLevel() const;

    std::unordered_map<std::string, Pandora::ResourceDataStoreSharedPtr> m_GenerationParameters;
    glm::vec2 m_SectorSize{ 10.0f, 10.0f };
};

} // namespace WingsOfSteel::TheBrightestStar