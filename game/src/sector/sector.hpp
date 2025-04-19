#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Encounter);
DECLARE_SMART_PTR(EncounterWindow);
DECLARE_SMART_PTR(Fleet);

DECLARE_SMART_PTR(Sector);
class Sector : public Pandora::Scene
{
public:
    Sector();
    ~Sector();

    void Initialize() override;
    void Update(float delta) override;

    void ShowCameraDebugUI(bool state);

    EncounterSharedPtr GetEncounter() const;
    void ForceEncounter(EncounterSharedPtr pEncounter);

    Pandora::EntitySharedPtr GetPlayerShip() const;

private:
    void DrawCameraDebugUI();
    void SpawnEncounter();
    void SpawnDome();
    void SpawnPlayerFleet();
    void GenerateDice();

    Pandora::EntitySharedPtr SpawnShip(const std::string& name, const std::string& modelPath);

    Pandora::EntitySharedPtr m_pDome;
    Pandora::EntitySharedPtr m_pCamera;
    FleetSharedPtr m_pPlayerFleet;
    Pandora::EntitySharedPtr m_pPlayerShip;
    bool m_ShowCameraDebugUI{false};
    EncounterWindowSharedPtr m_pEncounterWindow;
    EncounterSharedPtr m_pEncounter;
};

inline EncounterSharedPtr Sector::GetEncounter() const
{
    return m_pEncounter;
}

inline Pandora::EntitySharedPtr Sector::GetPlayerShip() const
{
    return m_pPlayerShip;
}

} // namespace WingsOfSteel::TheBrightestStar