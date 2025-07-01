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

    Pandora::EntitySharedPtr GetPlayerShip() const;
    Fleet* GetPlayerFleet() const;

private:
    void DrawCameraDebugUI();
    void SpawnDome();
    void SpawnPlayerFleet();

    Pandora::EntitySharedPtr m_pDome;
    Pandora::EntitySharedPtr m_pCamera;
    FleetSharedPtr m_pPlayerFleet;
    Pandora::EntitySharedPtr m_pPlayerShip;
    bool m_ShowCameraDebugUI{ false };
    bool m_ShowGrid{ true };
};

inline Pandora::EntitySharedPtr Sector::GetPlayerShip() const
{
    return m_pPlayerShip;
}

} // namespace WingsOfSteel::TheBrightestStar