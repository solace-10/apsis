#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Encounter);
DECLARE_SMART_PTR(Wing);

DECLARE_SMART_PTR(Sector);
class Sector : public Pandora::Scene
{
public:
    Sector();
    ~Sector();

    void Initialize() override;
    void Update(float delta) override;

    void ShowCameraDebugUI(bool state);
    void ShowGrid(bool state);

    Pandora::EntitySharedPtr GetPlayerMech() const;

private:
    void DrawCameraDebugUI();
    void SpawnDome();
    void SpawnPlayerFleet();

    EncounterUniquePtr m_pEncounter;
    Pandora::EntitySharedPtr m_pDome;
    Pandora::EntitySharedPtr m_pCamera;
    Pandora::EntityWeakPtr m_pPlayerMech;
    Pandora::EntityWeakPtr m_pCarrier;
    bool m_ShowCameraDebugUI{ false };
    bool m_ShowGrid{ true };
};

inline Pandora::EntitySharedPtr Sector::GetPlayerMech() const
{
    return m_pPlayerMech.lock();
}

} // namespace WingsOfSteel::TheBrightestStar