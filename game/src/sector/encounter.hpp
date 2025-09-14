#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

#include <array>
#include <vector>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Sector);
DECLARE_SMART_PTR(Wing);

DECLARE_SMART_PTR(Encounter);
class Encounter
{
public:
    Encounter();
    ~Encounter();

    void Initialize(SectorSharedPtr pSector);
    void Update(float delta);

private:
    struct EncounterWingDescription
    {
        std::vector<std::string> entities;
    };

    struct EncounterAction
    {
        std::vector<EncounterWingDescription> wings;
    };

    struct EncounterTier
    {
        std::vector<EncounterAction> actions;
        int currentAction{0};
    };

    void SpawnCarrier();
    void RebuildTier(int tier);
    void InstantiateAction(const EncounterAction& action);
    EntityWeakPtr m_pCarrier;
    std::vector<WingUniquePtr> m_Wings;

    SectorWeakPtr m_pSector;
    std::array<EncounterTier, 3> m_EncounterTiers;
    int m_CurrentTier{0};
    float m_TimeToNextAction{ 0.0f };
};

} // namespace WingsOfSteel