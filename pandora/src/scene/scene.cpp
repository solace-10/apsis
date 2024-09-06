#include "scene/scene.hpp"

#include "core/log.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/entity.hpp"
#include "scene/systems/system.hpp"

namespace WingsOfSteel::Pandora
{

Scene::Scene()
{

}

Scene::~Scene()
{

}

void Scene::Initialize()
{
    
}

void Scene::Update(float delta)
{
    ProcessEntitiesToAdd();

    for (auto& pSystem : m_Systems)
    {
        pSystem->Update(delta);
    }

    ProcessEntitiesToRemove();
}

EntitySharedPtr Scene::CreateEntity()
{
    EntitySharedPtr pEntity = std::make_shared<Entity>(this);
    m_EntitiesPendingAdd.push_back(pEntity);

    pEntity->m_EntityHandle = m_Registry.create();

    return pEntity;
}


void Scene::RemoveEntity(EntitySharedPtr pEntity)
{
    pEntity->m_MarkedForRemoval = true;
}

void Scene::AddSystem(SystemUniquePtr pSystem)
{
    pSystem->Initialize();
    m_Systems.push_back(std::move(pSystem));
}

void Scene::SetCamera(EntitySharedPtr pCamera)
{
    if (!pCamera->HasComponent<CameraComponent>())
    {
        Log::Error() << "Trying to add a camera without a CameraComponent.";
        m_pCamera.reset();
    }
    else if (!pCamera->HasComponent<TransformComponent>())
    {
        Log::Error() << "Trying to add a camera without a TransformComponent.";
        m_pCamera.reset();
    }
    else
    {
        m_pCamera = pCamera;
    }
}

EntitySharedPtr Scene::GetCamera() const
{
    return m_pCamera.lock();
}

void Scene::ProcessEntitiesToAdd()
{
    for (auto& pEntity : m_EntitiesPendingAdd)
    {
        m_Entities.push_back(pEntity);
        pEntity->OnAddedToScene();
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

entt::registry& Scene::GetRegistry()
{
    return m_Registry;
}

} // namespace WingsOfSteel::Pandoraaa