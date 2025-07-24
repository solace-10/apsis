#pragma once

/*

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/smart_ptr.hpp>
#include <scene/components/icomponent.hpp>


namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(EntityPrefabManager);
class EntityPrefabManager
{
public:
    EntityPrefabManager() = default;
    ~EntityPrefabManager() = default;

    class HardpointPrefab
    {
    public:
        HardpointPrefab() = default;
        ~HardpointPrefab() = default;

        std::string name;
        float minArc {0.0f};
        float maxArc {0.0f};
        std::string weapon;
    };

    enum class EntityType
    {
        Ship,
        Mech
    };

    DECLARE_SMART_PTR(EntityPrefab);
    class EntityPrefab
    {
    public:
        EntityPrefab() = default;
        ~EntityPrefab() = default;

        EntityType type;
        std::string name;
        std::string model;
        float hull {100.0f};
        float mass {1.0f};
        float engineForce {0.0f};
        float engineTorque {0.0f};
        std::vector<HardpointPrefab> hardpoints;
        std::vector<Pandora::IComponentUniquePtr> components;
    };

    using OnEntityPrefabLoadedCallback = std::function<void(const EntityPrefab*)>;
    void GetEntityPrefab(const std::string& resourcePath, OnEntityPrefabLoadedCallback onEntityPrefabLoaded);

private:
    std::unordered_map<std::string, EntityPrefabUniquePtr> m_EntityPrefabs;
};

} // namespace WingsOfSteel::TheBrightestStar

*/