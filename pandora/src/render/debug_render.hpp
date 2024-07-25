#pragma once

#include <memory>

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

namespace Private
{
    class DebugRenderImpl;
}

class DebugRender
{
public:
    DebugRender();
    ~DebugRender();

    void Render(wgpu::RenderPassEncoder& renderPass);

    void Point(const glm::vec3& pos, const glm::vec3& color, const float size, const int durationMillis = 0, const bool depthEnabled = true);
    void Line(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color, const int durationMillis = 0, const bool depthEnabled = true);

private:
    std::unique_ptr<Private::DebugRenderImpl> m_pImpl;
};

} // namespace WingsOfSteel::Pandora