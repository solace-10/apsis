#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SectorInfo);

DECLARE_SMART_PTR(Sector);
class Sector : public Pandora::Scene
{
public:
    Sector(SectorInfoSharedPtr& pSectorInfo);
    ~Sector();

    void Initialize() override;
    void Update(float delta) override;

    void ShowSignalsDebugUI(bool state);

private:
    void DrawSignalsDebugUI();

    SectorInfoSharedPtr m_pSectorInfo;
    Pandora::EntitySharedPtr m_pCamera;
    bool m_ShowSignalsDebugUI;
};

} // namespace WingsOfSteel::TheBrightestStar