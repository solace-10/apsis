#pragma once

#include <deque>
#include <optional>
#include <string>
#include <vector>

#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/systems/system.hpp>

#include "components/carrier_component.hpp"
#include "components/wing_component.hpp"

namespace WingsOfSteel
{

class CarrierSystem : public System
{
public:
    CarrierSystem() = default;
    ~CarrierSystem() = default;

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override;

    void LaunchEscorts(EntitySharedPtr pCarrierEntity, const std::vector<std::string>& escorts, WingRole role);

private:
    void InitializeLaunchWaypoints(CarrierComponent& carrierComponent, const ModelComponent& modelComponent);
    void DrawDebugLaunchWaypoints(const CarrierComponent& carrierComponent, const TransformComponent& transformComponent);
    void ProcessLaunchQueues(float delta);
    void UpdateLaunchSequences(float delta);
};

} // namespace WingsOfSteel