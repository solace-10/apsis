#pragma once

#include <memory>
#include <string>

#include <glm/mat4x4.hpp>
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

    // Add a point in 3D space. Point is expressed in world-space coordinates.
    void Point(const glm::vec3& pos, const Color& color, const float size, const int durationMillis = 0);

    // Add a 3D line. Note that lines are expressed in world coordinates, and so are all wireframe primitives which are built from lines.
    void Line(const glm::vec3& from, const glm::vec3& to, const Color& color, const int durationMillis = 0);

    // Add a 2D text string as an overlay to the current view, using a built-in font.
    // Position is in screen-space pixels, origin at the top-left corner of the screen.
    // The third element (Z) of the position vector is ignored.
    // Note: Newlines and tabs are handled (1 tab = 4 spaces).
    void ScreenText(const std::string& str, const glm::vec3& pos, const Color& color, float scaling = 1.0f, int durationMillis = 0);

    // Add a 3D text label centered at the given world position that gets projected to screen-space. The label always faces the viewer.
    void Label(const std::string& str, const glm::vec3& pos, const Color& color, float scaling = 1.0f, int durationMillis = 0);

    // Add a set of three coordinate axis depicting the position and orientation of the given transform.
    // 'size' defines the size of the arrow heads. 'length' defines the length of the arrow's base line.
    void AxisTriad(const glm::mat4x4& transform, float size, float length, int durationMillis = 0);

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

    // Add a wireframe frustum pyramid.
    // 'invClipMatrix' is the inverse of the matrix defining the frustum
    // (AKA clip) volume, which normally consists of the projection * view matrix.
    // E.g.: inverse(projMatrix * viewMatrix)
    void Frustum(const glm::mat4x4& invClipMatrix, const Color& color, int durationMillis = 0);

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