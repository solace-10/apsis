#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

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
    void ShowGrid(bool state);

    Pandora::EntitySharedPtr GetPlayerMech() const;
    Fleet* GetPlayerFleet() const;

private:
    void DrawCameraDebugUI();
    void SpawnDome();
    void SpawnPlayerFleet();
    void SpawnEnemyFleet();

    Pandora::EntitySharedPtr m_pDome;
    Pandora::EntitySharedPtr m_pCamera;
    FleetSharedPtr m_pPlayerFleet;
    FleetSharedPtr m_pEnemyFleet;
    Pandora::EntitySharedPtr m_pPlayerMech;
    bool m_ShowCameraDebugUI{ false };
    bool m_ShowGrid{ true };
};

inline Pandora::EntitySharedPtr Sector::GetPlayerMech() const
{
    return m_pPlayerMech;
}

} // namespace WingsOfSteel::TheBrightestStar