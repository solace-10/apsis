#pragma once

#include <LinearMath/btIDebugDraw.h>

#include "core/smart_ptr.hpp"
#include "physics/physics_visualization.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(BulletDebugRender);
class BulletDebugRender : public btIDebugDraw
{
public:
    BulletDebugRender() {}

    void SetEnabled(PhysicsVisualization::Mode mode, bool state);
    bool IsEnabled(PhysicsVisualization::Mode mode) const;

    void draw3dText(const btVector3& location, const char* pTextString) override;
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    void drawSphere(const btVector3& p, btScalar radius, const btVector3& color) override;
    void drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
    void reportErrorWarning(const char* pWarningString) override;
    void setDebugMode(int debugMode) override;
    int getDebugMode() const override;

private:
    int32_t ToBulletFlag(PhysicsVisualization::Mode mode) const;

    int32_t m_BulletDebugMode{ 0 };
    int32_t m_DebugMode{ 0 };
};

} // namespace WingsOfSteel::Pandora