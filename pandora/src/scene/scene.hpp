#pragma once

#include <vector>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(Scene);
DECLARE_SMART_PTR(Entity);
DECLARE_SMART_PTR(Camera);

class Scene
{
public:
    Scene();
    ~Scene();

    void Update(float delta);
    void Render();

    void AddEntity(EntitySharedPtr pEntity);
    void RemoveEntity(EntitySharedPtr pEntity);
    void SetCamera(CameraSharedPtr pCamera);

private:
    void ProcessEntitiesToAdd();
    void ProcessEntitiesToRemove();

    std::vector<EntitySharedPtr> m_Entities;
    std::vector<EntitySharedPtr> m_EntitiesPendingAdd;
    CameraSharedPtr m_pCamera;
};

} // namespace WingsOfSteel::Pandoraaa