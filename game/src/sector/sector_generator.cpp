#include <sstream>
#include <vector>

#include <core/log.hpp>
#include <core/random.hpp>
#include <pandora.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>

#include "sector/signal/asteroid_field_signal.hpp"
#include "sector/sector_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

SectorGenerator::SectorGenerator()
{
    LoadGenerationParameters();
}

SectorGenerator::~SectorGenerator()
{

}

SectorInfoSharedPtr SectorGenerator::Create(uint32_t level)
{
    const Pandora::ResourceDataStoreSharedPtr& generationParameters = m_GenerationParameters[GetPathForLevel(level)];

    std::vector<SignalSharedPtr> signals = GenerateSignals(generationParameters);

    return nullptr;
}

bool SectorGenerator::CanCreate(uint32_t level) const
{
    return m_GenerationParameters.find(GetPathForLevel(level)) != m_GenerationParameters.end();
}

std::string SectorGenerator::GetPathForLevel(uint32_t level) const
{
    std::stringstream path;
    path << "/sector_generator/hyperscape_depth_" << level << ".json";
    return path.str();
}

void SectorGenerator::LoadGenerationParameters()
{
    using namespace Pandora;
    Log::Info() << "Sector generator: loading generation parameters...";
    for (int i = 0; i < 100; i++)
    {
        std::string path(GetPathForLevel(i));
        if (GetVFS()->Exists(path))
        {
            GetResourceSystem()->RequestResource(path, [this](ResourceSharedPtr pResource) {
                this->m_GenerationParameters[pResource->GetPath()] = std::dynamic_pointer_cast<Pandora::ResourceDataStore>(pResource);
            });
        }
        else
        {
            break;
        }
    }
}

std::vector<SignalSharedPtr> SectorGenerator::GenerateSignals(const Pandora::ResourceDataStoreSharedPtr& generationParameters) const
{
    std::vector<SignalSharedPtr> signals;

    const nlohmann::json& signalsParameters = generationParameters->Data()["signals"];
    int minimumSignals = signalsParameters["minimum"].get<int>();
    int maximumSignals = signalsParameters["maximum"].get<int>();
    int numSignals = Pandora::Random::Get(minimumSignals, maximumSignals);
    const nlohmann::json& spawnChances = signalsParameters["spawn_chance"];

    for (int i = 0; i < numSignals; i++)
    {
        const float roll = Pandora::Random::Get(0.0f, 1.0f);
        float accum = 0.0f;
        for (int j = 0; j < spawnChances.size(); j++)
        {
            accum += spawnChances[j]["chance"].get<float>();
            if (roll <= accum)
            {
                const std::string& signalType = spawnChances[j]["signal"].get<std::string>();
                signals.push_back(GenerateSignal(signalType, generationParameters->Data()[signalType]));
                break;
            }
        }
    }

    return signals;
}

SignalSharedPtr SectorGenerator::GenerateSignal(const std::string& signalType, const nlohmann::json& generationParameters) const
{
    if (signalType == "asteroid_field") return GenerateAsteroidFieldSignal(generationParameters);

    Pandora::Log::Error() << "SectorGenerator::GenerateSignal failed to create signal type '" << signalType << '.';
    return SignalSharedPtr();
}

SignalSharedPtr SectorGenerator::GenerateAsteroidFieldSignal(const nlohmann::json& generationParameters) const
{
    const float roll = Pandora::Random::Get(0.0f, 1.0f);
    float accum = 0.0f;
    for (int i = 0; i < generationParameters.size(); i++)
    {
        accum += generationParameters[i]["spawn_chance"].get<float>();
        if (roll <= accum)
        {
            AsteroidFieldSignalSharedPtr pAsteroidFieldSignal = std::make_shared<AsteroidFieldSignal>(GenerateSignalPosition(), generationParameters[i]);
            pAsteroidFieldSignal->AddContent("todo", 0.0f);
            return pAsteroidFieldSignal;
        }
    }

    Pandora::Log::Error() << "SectorGenerator::GenerateAsteroidFieldSignal failed.";
    return SignalSharedPtr();  
}

glm::vec3 SectorGenerator::GenerateSignalPosition() const
{
    float m = 100.0f;
    float x = Pandora::Random::Get(-m, m);
    float z = Pandora::Random::Get(-m, m);
    return glm::vec3(x, 0.0f, z);
}

SectorCivilizationLevel SectorGenerator::GenerateCivilizationLevel() const
{
    uint32_t roll = Pandora::Random::Get(100);

    // Uncolonized, // No life has ever been present in this sector.
    // Decivilized, // Something catastrophic has happened here: it used to be colonized, but all that remains are ruins.
    // Outposts,    // Minimal presence, perhaps a research or military outpost but no large colonies.
    // Populated,   // One or more worlds are well populated.
    // Colonized,   // The entire system has been colonized.

    // std::vector<std::pair<SectorCivilizationLevel, uint32_t>> map =
    // {
    //     { SectorCivilizationLevel::Decivilized, 15 },
    //     { SectorCivilizationLevel::Outposts, 15 },
    //     { SectorCivilizationLevel::Populated, 20 },
    //     { SectorCivilizationLevel::Colonized, 10 }
    // };
    return SectorCivilizationLevel::Uncolonized;
}

} // namespace WingsOfSteel::TheBrightestStar