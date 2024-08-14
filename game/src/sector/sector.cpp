#include "sector/sector_camera.hpp"
#include "sector/sector_info.hpp"
#include "sector/sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Sector::Sector(SectorInfoSharedPtr& pSectorInfo)
: m_pSectorInfo(pSectorInfo)
{

}

Sector::~Sector()
{

}

void Sector::Initialize()
{
    Pandora::Scene::Initialize();

    m_pCamera = std::make_shared<SectorCamera>();
    m_pCamera->LookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    AddEntity(m_pCamera);
    SetCamera(m_pCamera);
}

void Sector::Update(float delta)
{
    Pandora::Scene::Update(delta);
}

} // namespace WingsOfSteel::TheBrightestStar