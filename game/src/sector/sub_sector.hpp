#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SubSectorInfo);

DECLARE_SMART_PTR(SubSector);
class SubSector : public Pandora::Scene
{
public:
    SubSector(SubSectorInfoSharedPtr& pSubSectorInfo);
    ~SubSector();

    void Initialize() override;
    void Update(float delta) override;

    void ShowSignalsDebugUI(bool state);

private:
    void DrawSignalsDebugUI();
    void SpawnPlayerShip();

    SubSectorInfoSharedPtr m_pSubSectorInfo;
    Pandora::EntitySharedPtr m_pCamera;
    bool m_ShowSignalsDebugUI;
};

} // namespace WingsOfSteel::TheBrightestStar