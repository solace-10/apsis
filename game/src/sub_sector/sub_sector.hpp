#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SubSector);
class SubSector : public Pandora::Scene
{
public:
    SubSector();
    ~SubSector();

    void Initialize() override;
    void Update(float delta) override;

    //void ShowSignalsDebugUI(bool state);

private:
    //void DrawSignalsDebugUI();
    void SpawnPlayerShip();

    //Sector* m_pSector;
    //SectorSignal* m_pSectorSignal;
    Pandora::EntitySharedPtr m_pCamera;
    Pandora::EntitySharedPtr m_pPlayerShip;
    //bool m_ShowSignalsDebugUI;
};

} // namespace WingsOfSteel::TheBrightestStar