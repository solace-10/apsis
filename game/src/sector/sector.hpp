#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SectorCamera);
DECLARE_SMART_PTR(SectorInfo);

DECLARE_SMART_PTR(Sector);
class Sector : public Pandora::Scene
{
public:
    Sector(SectorInfoSharedPtr& pSectorInfo);
    ~Sector();

    void Initialize() override;
    void Update(float delta) override;

private:
    SectorInfoSharedPtr m_pSectorInfo;
    SectorCameraSharedPtr m_pCamera;
};

} // namespace WingsOfSteel::TheBrightestStar