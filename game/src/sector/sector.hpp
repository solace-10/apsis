#pragma once

#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

#include <core/signal.hpp>
#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel
{

namespace Json
{
using Data = nlohmann::json;
}

DECLARE_SMART_PTR(Database);
DECLARE_SMART_PTR(GroupFilters);

DECLARE_SMART_PTR(Sector);
class Sector : public Scene
{
public:
    Sector();
    ~Sector();

    void Initialize() override;
    void Update(float delta) override;

    void InitializeSpaceObjects(const Json::Data& objectsData, const Json::Data& groupsData);

    void ShowCameraDebugUI(bool state);
    void ShowGrid(bool state);

    Database* GetDatabase() { return m_pDatabase.get(); }
    GroupFilters* GetGroupFilters() { return m_pGroupFilters.get(); }
    EntitySharedPtr GetEarth() const { return m_pEarth; }
    EntitySharedPtr GetSelectedSpaceObject() const { return m_pSelectedSpaceObject.lock(); }
    void SetSelectedSpaceObject(EntitySharedPtr pEntity);

    EntitySharedPtr GetEntityByNoradId(uint32_t noradId) const;

private:
    void DrawCameraDebugUI();
    void SpawnLight();
    void InitializeDatabase();
    void InitializeGroupFilters();
    void InitializeOtherGroupFilter();

    DatabaseUniquePtr m_pDatabase;
    EntitySharedPtr m_pCamera;
    EntitySharedPtr m_pLight;
    EntitySharedPtr m_pEarth;
    bool m_ShowCameraDebugUI{ false };
    bool m_ShowGrid{ false };
    EntityWeakPtr m_pSelectedSpaceObject;
    std::vector<EntitySharedPtr> m_NoradIdIndex;
    GroupFiltersUniquePtr m_pGroupFilters;
};

} // namespace WingsOfSteel
