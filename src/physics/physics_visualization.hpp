#pragma once

#include <memory>

#include "core/smart_ptr.hpp"

class btCollisionWorld;

namespace WingsOfSteel
{

class BulletDebugRender;

DECLARE_SMART_PTR(PhysicsVisualization);
class PhysicsVisualization
{
public:
    PhysicsVisualization(btCollisionWorld* pWorld);
    ~PhysicsVisualization();

    void Update();

    // clang-format off
    enum class Mode
    {
        None          = 0,
        Wireframe     = 1 << 0,
        AABB          = 1 << 1,
        Transforms    = 1 << 2,
        RayTests      = 1 << 3,
        ContactPoints = 1 << 4
    };
    // clang-format on

    void SetEnabled(Mode mode, bool state);
    bool IsEnabled(Mode mode) const;

private:
    std::unique_ptr<BulletDebugRender> m_pDebugRender;
    btCollisionWorld* m_pWorld{ nullptr };
};

} // namespace WingsOfSteel