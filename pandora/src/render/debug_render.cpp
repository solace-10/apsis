#include "render/private/debug_render_impl.hpp"
#include "render/debug_render.hpp"

namespace WingsOfSteel::Pandora
{

DebugRender::DebugRender()
{
    m_pImpl = std::make_unique<Private::DebugRenderImpl>();
    m_pImpl->Initialize();
    dd::initialize(m_pImpl.get());
}

DebugRender::~DebugRender()
{
    dd::shutdown();
}

void DebugRender::Render(wgpu::RenderPassEncoder& renderPass)
{
    m_pImpl->Render(renderPass);
}

void DebugRender::Point(const glm::vec3& pos, const glm::vec3& color, const float size, const int durationMillis, const bool depthEnabled)
{
    dd::point(pos, color, size, durationMillis, depthEnabled);
}

void DebugRender::Line(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color, const int durationMillis, const bool depthEnabled)
{
    dd::line(from, to, color, durationMillis, depthEnabled);
}

} // namespace WingsOfSteel::Pandora