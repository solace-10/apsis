#pragma once

#include <array>
#include <deque>
#include <optional>

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

#include "components/wing_component.hpp"

namespace WingsOfSteel
{

class EscortToLaunch
{
public:
    EscortToLaunch(const std::string& resourcePath, WingID wingID, WingRole wingRole)
    : m_ResourcePath(resourcePath)
    , m_WingID(wingID)
    , m_WingRole(wingRole)
    {
    }

const std::string& GetResourcePath() const { return m_ResourcePath; }
WingID GetWingID() const { return m_WingID; }
WingRole GetWingRole() const { return m_WingRole; }

private:
    std::string m_ResourcePath;
    WingID m_WingID;
    WingRole m_WingRole;
};

class CarrierComponent : public IComponent
{
public:
    CarrierComponent() 
    {
        TargetRolePercentages[static_cast<size_t>(WingRole::Defense)] = 0.4f;
        TargetRolePercentages[static_cast<size_t>(WingRole::Offense)] = 0.4f;
        TargetRolePercentages[static_cast<size_t>(WingRole::Interception)] = 0.2f;
    }

    ~CarrierComponent() = default;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
    }

    std::array<float, static_cast<size_t>(WingRole::Count)> TargetRolePercentages{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, static_cast<size_t>(WingRole::Count)> CurrentRolePercentages{ 0.0f, 0.0f, 0.0f, 0.0f };
    WingID CurrentWingID{ 0 };
    std::deque<EscortToLaunch> QueuedLaunches;
    std::optional<EscortToLaunch> CurrentLaunch;
    float TimeToNextLaunch{0.0f};
};

REGISTER_COMPONENT(CarrierComponent, "carrier")

} // namespace WingsOfSteel