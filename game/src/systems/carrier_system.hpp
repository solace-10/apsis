#pragma once

#include <deque>
#include <optional>
#include <string>
#include <vector>

#include <scene/systems/system.hpp>

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

};

} // namespace WingsOfSteel