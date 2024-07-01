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

    for (auto& pEntity : m_Entities)
    {
        pEntity->Update(delta);   
    }

    ProcessEntitiesToRemove();
}

void Scene::Render(wgpu::RenderPassEncoder renderPass)
{
    for (auto& pEntity : m_Entities)
    {
        pEntity->Render(renderPass);
    }
}

void Scene::AddEntity(EntitySharedPtr pEntity)
{
    m_EntitiesPendingAdd.push_back(pEntity);
}
    
void Scene::RemoveEntity(EntitySharedPtr pEntity)
{
    pEntity->m_MarkedForRemoval = true;
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
        pEntity->OnAddedToScene(this);
    }
    m_EntitiesPendingAdd.clear();
}

void Scene::ProcessEntitiesToRemove()
{
    for (auto& pEntity : m_Entities)
    {
        if (pEntity->m_MarkedForRemoval)
        {
            pEntity->OnRemovedFromScene();
        }
    }

    std::erase_if(m_Entities, [](EntitySharedPtr pEntity) { return pEntity->m_MarkedForRemoval; });
}

} // namespace WingsOfSteel::Pandoraaa