#include <glm/gtc/type_ptr.hpp>

#include "pandora.hpp"
#include "render/debug_render.hpp"
#include "render/debug_render_demo.hpp"
#include "render/private/debug_render_impl.hpp"
#include "render/window.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel
{

DebugRender::DebugRender()
{
    m_pImpl = std::make_unique<Private::DebugRenderImpl>();
    m_pImpl->Initialize();

    m_pDemo = std::make_unique<DebugRenderDemo>();
}

DebugRender::~DebugRender()
{
}

DebugRenderDemo* DebugRender::GetDemo() const
{
    return m_pDemo.get();
}

void DebugRender::Update(float delta)
{
    m_pDemo->Update(delta);
}

void DebugRender::Render(wgpu::RenderPassEncoder& renderPass)
{
    m_pImpl->Render(renderPass);
}

void DebugRender::Point(const glm::vec3& pos, const Color& color, const float size, const int durationMillis)
{
    dd::point(pos, color.AsVec3(), size, durationMillis);
}

void DebugRender::Line(const glm::vec3& from, const glm::vec3& to, const Color& color, const int durationMillis)
{
    dd::line(from, to, color.AsVec3(), durationMillis);
}

void DebugRender::ScreenText(const std::string& str, const glm::vec3& pos, const Color& color, float scaling, int durationMillis)
{
    dd::screenText(str.c_str(), pos, color.AsVec3(), scaling, durationMillis);
}

void DebugRender::Label(const std::string& str, const glm::vec3& pos, const Color& color, float scaling, int durationMillis)
{
    EntitySharedPtr pCamera = GetActiveScene() ? GetActiveScene()->GetCamera() : nullptr;
    if (pCamera)
    {
        CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
        const glm::mat4 viewProjection = cameraComponent.camera.GetProjectionMatrix() * cameraComponent.camera.GetViewMatrix();
        dd::projectedText(str.c_str(), pos, color.AsVec3(), glm::value_ptr(viewProjection), 0, 0, GetWindow()->GetWidth(), GetWindow()->GetHeight(), scaling, durationMillis);
    }
}

void DebugRender::AxisTriad(const glm::mat4x4& transform, float size, float length, int durationMillis)
{
    dd::axisTriad(glm::value_ptr(transform), size, length, durationMillis);
}

void DebugRender::Arrow(const glm::vec3& from, const glm::vec3& to, const Color& color, float size, int durationMillis)
{
    dd::arrow(from, to, color.AsVec3(), size, durationMillis);
}

void DebugRender::Cross(const glm::vec3& center, float length, int durationMillis)
{
    dd::cross(center, length, durationMillis);
}

void DebugRender::Circle(const glm::vec3& center, const glm::vec3& planeNormal, const Color& color, float radius, int numSteps, int durationMillis)
{
    dd::circle(center, planeNormal, color.AsVec3(), radius, static_cast<float>(numSteps), durationMillis);
}

void DebugRender::Plane(const glm::vec3& center, const glm::vec3& planeNormal, const Color& planeColor, const Color& normalVecColor, float planeScale, float normalVecScale, int durationMillis)
{
    dd::plane(center, planeNormal, planeColor.AsVec3(), normalVecColor.AsVec3(), planeScale, normalVecScale, durationMillis);
}

void DebugRender::Sphere(const glm::vec3& center, const Color& color, float radius, int durationMillis)
{
    dd::sphere(center, color.AsVec3(), radius, durationMillis);
}

void DebugRender::Cone(const glm::vec3& apex, const glm::vec3& dir, const Color& color, float baseRadius, float apexRadius, int durationMillis)
{
    dd::cone(apex, dir, color.AsVec3(), baseRadius, apexRadius, durationMillis);
}

void DebugRender::Box(const glm::vec3 points[8], const Color& color, int durationMillis)
{
    dd::box(points, color.AsVec3(), durationMillis);
}

void DebugRender::Box(const glm::vec3& center, const Color& color, float width, float height, float depth, int durationMillis)
{
    dd::box(center, color.AsVec3(), width, height, depth, durationMillis);
}

void DebugRender::Aabb(const glm::vec3& mins, const glm::vec3& maxs, const Color& color, int durationMillis)
{
    dd::aabb(mins, maxs, color.AsVec3(), durationMillis);
}

void DebugRender::Frustum(const glm::mat4x4& invClipMatrix, const Color& color, int durationMillis)
{
    dd::frustum(glm::value_ptr(invClipMatrix), color.AsVec3(), durationMillis);
}

void DebugRender::VertexNormal(const glm::vec3& origin, const glm::vec3& normal, float length, int durationMillis)
{
    dd::vertexNormal(origin, normal, length, durationMillis);
}

void DebugRender::TangentBasis(const glm::vec3& origin, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, float lengths, int durationMillis)
{
    dd::tangentBasis(origin, normal, tangent, bitangent, lengths, durationMillis);
}

void DebugRender::XZSquareGrid(float mins, float maxs, float y, float step, const Color& color, int durationMillis)
{
    dd::xzSquareGrid(mins, maxs, y, step, color.AsVec3(), durationMillis);
}

} // namespace WingsOfSteel