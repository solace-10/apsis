#pragma once

#include <memory>

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/color.hpp"
#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

namespace Private
{
    class DebugRenderImpl;
}

class DebugRenderDemo;

class DebugRender
{
public:
    DebugRender();
    ~DebugRender();

    DebugRenderDemo* GetDemo() const;

    void Update(float delta);
    void Render(wgpu::RenderPassEncoder& renderPass);

    void Point(const glm::vec3& pos, const Color& color, const float size, const int durationMillis = 0);

    void Line(const glm::vec3& from, const glm::vec3& to, const Color& color, const int durationMillis = 0);

// void screenText(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
//                 const char * str,
//                 const glm::vec3& pos,
//                 const glm::vec3& color,
//                 float scaling = 1.0f,
//                 int durationMillis = 0);

// Add a 3D text label centered at the given world position that
// gets projected to screen-space. The label always faces the viewer.
// sx/sy, sw/sh are the viewport coordinates/size, in pixels.
// 'vpMatrix' is the view * projection transform to map the text from 3D to 2D.
// void projectedText(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
//                    const char * str,
//                    const glm::vec3& pos,
//                    const glm::vec3& color,
//                    ddMat4x4_In vpMatrix,
//                    int sx, int sy,
//                    int sw, int sh,
//                    float scaling = 1.0f,
//                    int durationMillis = 0);

// Add a set of three coordinate axis depicting the position and orientation of the given transform.
// 'size' defines the size of the arrow heads. 'length' defines the length of the arrow's base line.
// void axisTriad(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
//                ddMat4x4_In transform,
//                float size,
//                float length,
//                int durationMillis = 0,
//                bool depthEnabled = true);

    // Add a 3D line with an arrow-like end.
    // 'size' defines the arrow head size.
    void Arrow(const glm::vec3& from, const glm::vec3& to, const Color& color, float size, int durationMillis = 0);

    // Add an axis-aligned cross (3 lines converging at a point).
    // 'length' defines the length of the crossing lines.
    // 'center' is the world-space point where the lines meet.
    void Cross(const glm::vec3& center, float length, int durationMillis = 0);

    // Add a wireframe circle.
    void Circle(const glm::vec3& center, const glm::vec3& planeNormal, const Color& color, float radius, float numSteps, int durationMillis = 0);

    // Add a wireframe plane in 3D space.
    // If 'normalVecScale' is not zero, a line depicting the plane normal is also draw.
    void Plane(const glm::vec3& center, const glm::vec3& planeNormal, const Color& planeColor, const Color& normalVecColor, float planeScale, float normalVecScale, int durationMillis = 0);

    // Add a wireframe sphere.
    void Sphere(const glm::vec3& center, const Color& color, float radius, int durationMillis = 0);

    // Add a wireframe cone.
    // The cone 'apex' is the point where all lines meet.
    // The length of the 'dir' vector determines the thickness.
    // 'baseRadius' & 'apexRadius' are in degrees.
    void Cone(const glm::vec3& apex, const glm::vec3& dir, const Color& color, float baseRadius, float apexRadius, int durationMillis = 0);

    // Wireframe box from the eight points that define it.
    void Box(const glm::vec3 points[8], const Color& color, int durationMillis = 0);

    // Add a wireframe box.
    void Box(const glm::vec3& center, const Color& color, float width, float height, float depth, int durationMillis = 0);

    // Add a wireframe Axis Aligned Bounding Box (AABB).
    void Aabb(const glm::vec3& mins, const glm::vec3& maxs, const Color& color, int durationMillis = 0);

// // Add a wireframe frustum pyramid.
// // 'invClipMatrix' is the inverse of the matrix defining the frustum
// // (AKA clip) volume, which normally consists of the projection * view matrix.
// // E.g.: inverse(projMatrix * viewMatrix)
// void frustum(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
//              ddMat4x4_In invClipMatrix,
//              const glm::vec3& color,
//              int durationMillis = 0,
//              bool depthEnabled = true);

    // Add a vertex normal for debug visualization.
    // The normal vector 'normal' is assumed to be already normalized.
    void VertexNormal(const glm::vec3& origin, const glm::vec3& normal, float length, int durationMillis = 0);

    // Add a "tangent basis" at a given point in world space.
    // Color scheme used is: normal=WHITE, tangent=YELLOW, bi-tangent=MAGENTA.
    // The normal vector, tangent and bi-tangent vectors are assumed to be already normalized.
    void TangentBasis(const glm::vec3& origin, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, float lengths, int durationMillis = 0);

    // Makes a 3D square grid of lines along the X and Z planes.
    // 'y' defines the height in the Y axis where the grid is placed.
    // The grid will go from 'mins' to 'maxs' units in both the X and Z.
    // 'step' defines the gap between each line of the grid.
    void XZSquareGrid(float mins, float maxs, float y, float step, const Color& color, int durationMillis = 0);

private:
    std::unique_ptr<Private::DebugRenderImpl> m_pImpl;
    std::unique_ptr<DebugRenderDemo> m_pDemo;  
};

} // namespace WingsOfSteel::Pandora