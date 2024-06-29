#include "scene/scene.hpp"

#include "scene/camera.hpp"
#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

Scene::Scene()
{

}

Scene::~Scene()
{

}

void Scene::Update(float delta)
{
    ProcessEntitiesToAdd();
    ProcessEntitiesToRemove();

    for (auto& pEntity : m_Entities)
    {
        pEntity->Update(delta);   
    }
}

void Scene::Render()
{
    for (auto& pEntity : m_Entities)
    {
        pEntity->Render();
    }
}

void Scene::AddEntity(EntitySharedPtr pEntity)
{
    m_EntitiesPendingAdd.push_back(pEntity);
}
    
void Scene::RemoveEntity(EntitySharedPtr pEntity)
{
    assert(false); // ProcessEntitiesToRemove() not implemented
    m_EntitiesPendingRemove.push_back(pEntity);
}

void Scene::SetCamera(CameraSharedPtr pCamera)
{
    m_pCamera = pCamera;
}

void Scene::ProcessEntitiesToAdd()
{
    for (auto& pEntity : m_EntitiesPendingAdd)
    {
        m_Entities.push_back(pEntity);
    }
    m_EntitiesPendingAdd.clear();
}

void Scene::ProcessEntitiesToRemove()
{

}

} // namespace WingsOfSteel::Pandoraaa