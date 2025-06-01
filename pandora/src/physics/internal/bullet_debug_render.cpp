#include "physics/internal/bullet_debug_render.hpp"

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/debug_render.hpp"

namespace WingsOfSteel::Pandora
{

void BulletDebugRender::SetEnabled(PhysicsVisualization::Mode mode, bool state)
{
    const int32_t f = ToBulletFlag(mode);
    if (state)
    {
        m_DebugMode |= static_cast<int>(mode);
        m_BulletDebugMode |= f;
    }
    else
    {
        m_DebugMode &= ~static_cast<int>(mode);
        m_BulletDebugMode &= ~f;
    }
}

bool BulletDebugRender::IsEnabled(PhysicsVisualization::Mode mode) const
{
    return (m_DebugMode & static_cast<int>(mode)) > 0;
}

int32_t BulletDebugRender::ToBulletFlag(PhysicsVisualization::Mode mode) const
{
    switch (mode)
    {
    case PhysicsVisualization::Mode::Wireframe:
        return DBG_DrawWireframe;
    case PhysicsVisualization::Mode::AABB:
        return DBG_DrawAabb;
    default:
        return 0;
    };
}

void BulletDebugRender::draw3dText(const btVector3& location, const char* pTextString)
{
    Pandora::GetDebugRender()->ScreenText(
        std::string(pTextString),
        glm::vec3(location.x(), location.y(), location.z()),
        Color::White);
}

void BulletDebugRender::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
    Pandora::GetDebugRender()->Line(
        glm::vec3(from.x(), from.y(), from.z()),
        glm::vec3(to.x(), to.y(), to.z()),
        Color(color.x(), color.y(), color.z()));
}

void BulletDebugRender::drawSphere(const btVector3& p, btScalar radius, const btVector3& color)
{
    Pandora::GetDebugRender()->Sphere(
        glm::vec3(p.x(), p.y(), p.z()),
        Color(color.x(), color.y(), color.z()),
        radius);
}

void BulletDebugRender::drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
}

void BulletDebugRender::reportErrorWarning(const char* pWarningString)
{
    Log::Warning() << "[Bullet]: " << pWarningString;
}

void BulletDebugRender::setDebugMode(int32_t debugMode)
{
    m_BulletDebugMode = debugMode;
}

int32_t BulletDebugRender::getDebugMode() const
{
    return m_BulletDebugMode;
}

} // namespace WingsOfSteel::Pandora