#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Encounter);
DECLARE_SMART_PTR(Wing);

DECLARE_SMART_PTR(Sector);
class Sector : public Scene
{
public:
    Sector();
    ~Sector();

    void Initialize() override;
    void Update(float delta) override;

    void ShowCameraDebugUI(bool state);
    void ShowGrid(bool state);

    Encounter* GetEncounter();
    EntitySharedPtr GetPlayerMech() const;

private:
    void DrawCameraDebugUI();
    void SpawnDome();
    void SpawnPlayerFleet();

    EncounterUniquePtr m_pEncounter;
    EntitySharedPtr m_pDome;
    EntitySharedPtr m_pCamera;
    EntityWeakPtr m_pPlayerMech;
    EntityWeakPtr m_pCarrier;
    bool m_ShowCameraDebugUI{ false };
    bool m_ShowGrid{ true };
};

inline Encounter* Sector::GetEncounter()
{
    return m_pEncounter.get();
}

inline EntitySharedPtr Sector::GetPlayerMech() const
{
    return m_pPlayerMech.lock();
}

} // namespace WingsOfSteel