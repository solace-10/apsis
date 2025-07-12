#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/smart_ptr.hpp>


namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(ShipPrefabManager);
class ShipPrefabManager
{
public:
    ShipPrefabManager() = default;
    ~ShipPrefabManager() = default;

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

    DECLARE_SMART_PTR(ShipPrefab);
    class ShipPrefab
    {
    public:
        ShipPrefab() = default;
        ~ShipPrefab() = default;

        std::string name;
        std::string model;
        float hull {100.0f};
        float mass {1.0f};
        float engineForce {0.0f};
        float engineTorque {0.0f};
        std::vector<HardpointPrefab> hardpoints;
    };

    using OnShipPrefabLoadedCallback = std::function<void(const ShipPrefab*)>;
    void GetShipPrefab(const std::string& resourcePath, OnShipPrefabLoadedCallback onShipPrefabLoaded);

private:
    std::unordered_map<std::string, ShipPrefabUniquePtr> m_ShipPrefabs;
};

} // namespace WingsOfSteel::TheBrightestStar