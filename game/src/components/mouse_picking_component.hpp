#pragma once

#include <glm/vec2.hpp>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class MousePickingComponent : public IComponent
{
public:
    MousePickingComponent() = default;
    ~MousePickingComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    void SetScreenSpacePosition(const glm::vec2& position) { m_ScreenSpacePosition = position; }
    const glm::vec2& GetScreenSpacePosition() const { return m_ScreenSpacePosition; }
    void SetEnabled(bool isEnabled) { m_IsEnabled = isEnabled; }
    bool IsEnabled() const { return m_IsEnabled; }

private:
    glm::vec2 m_ScreenSpacePosition{ 0.0f };
    bool m_IsEnabled{ true };
};

REGISTER_COMPONENT(MousePickingComponent, "mouse_picking")

} // namespace WingsOfSteel
