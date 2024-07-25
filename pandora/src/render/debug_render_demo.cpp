#include "core/color.hpp"
#include "render/debug_render.hpp"
#include "render/debug_render_demo.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

DebugRenderDemo::DebugRenderDemo()
: m_Show(false)
{

}
    
DebugRenderDemo::~DebugRenderDemo()
{

}
    
void DebugRenderDemo::Show(bool state)
{
    m_Show = state;
}

void DebugRenderDemo::Update(float delta)
{
    if (!m_Show)
    {
        return;
    }

    DrawGrid();
    DrawMiscObjects();
    DrawFrustum();
    DrawText();
}

void DebugRenderDemo::DrawGrid()
{
    // Grid from -50 to +50 in both X & Z
    GetDebugRender()->XZSquareGrid(-50.0f, 50.0f, -1.0f, 1.7f, Color::Green);
}

void DebugRenderDemo::DrawLabel(const glm::vec3& pos, const std::string& name)
{
    // const ddVec3 textColor = { 0.8f, 0.8f, 1.0f };
    // dd::projectedText(ctx, name, pos, textColor, toFloatPtr(camera.vpMatrix),
    //                     0, 0, WindowWidth, WindowHeight, 0.5f);
}

void DebugRenderDemo::DrawMiscObjects()
{
    // Start a row of objects at this position:
    glm::vec3 origin(-15.0f, 0.0f, 0.0f);

    // Box with a point at it's center:
    DrawLabel(origin, "box");
    GetDebugRender()->Box(origin, Color::Blue, 1.5f, 1.5f, 1.5f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);
    origin[0] += 3.0f;

    // Sphere with a point at its center
    DrawLabel(origin, "sphere");
    GetDebugRender()->Sphere(origin, Color::Red, 1.0f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);
    origin[0] += 4.0f;

    // Two cones, one open and one closed:
    const glm::vec3 coneDir(0.0f, 2.5f, 0.0f);
    origin[1] -= 1.0f;

    DrawLabel(origin, "cone (open)");
    GetDebugRender()->Cone(origin, coneDir, Color::Yellow, 1.0f, 2.0f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);
    origin[0] += 4.0f;

    DrawLabel(origin, "cone (closed)");
    GetDebugRender()->Cone(origin, coneDir, Color::Cyan, 0.0f, 1.0f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);
    origin[0] += 4.0f;

    // Axis-aligned bounding box:
    const glm::vec3 bbMins(-1.0f, -0.9f, -1.0f);
    const glm::vec3 bbMaxs(1.0f,  2.2f,  1.0f);
    const glm::vec3 bbCenter(
        (bbMins[0] + bbMaxs[0]) * 0.5f,
        (bbMins[1] + bbMaxs[1]) * 0.5f,
        (bbMins[2] + bbMaxs[2]) * 0.5f
    );
    DrawLabel(origin, "AABB");
    GetDebugRender()->Aabb(bbMins, bbMaxs, Color::Orange);
    GetDebugRender()->Point(bbCenter, Color::White, 15.0f);

    // Move along the Z for another row:
    origin[0] = -15.0f;
    origin[2] += 5.0f;

    // A big arrow pointing up:
    const glm::vec3 arrowFrom(origin[0], origin[1], origin[2]);
    const glm::vec3 arrowTo(origin[0], origin[1] + 5.0f, origin[2]);
    DrawLabel(arrowFrom, "arrow");
    GetDebugRender()->Arrow(arrowFrom, arrowTo, Color::Magenta, 1.0f);
    GetDebugRender()->Point(arrowFrom, Color::White, 15.0f);
    GetDebugRender()->Point(arrowTo, Color::White, 15.0f);
    origin[0] += 4.0f;

    // Plane with normal vector:
    const glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);
    DrawLabel(origin, "plane");
    GetDebugRender()->Plane(origin, planeNormal, Color::Yellow, Color::Blue, 1.5f, 1.0f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);
    origin[0] += 4.0f;

    // Circle on the Y plane:
    DrawLabel(origin, "circle");
    GetDebugRender()->Circle(origin, planeNormal, Color::Orange, 1.5f, 15.0f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);
    origin[0] += 3.2f;

    // Tangent basis vectors:
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    const glm::vec3 bitangent(0.0f, 0.0f, 1.0f);
    origin[1] += 0.1f;
    DrawLabel(origin, "tangent basis");
    GetDebugRender()->TangentBasis(origin, normal, tangent, bitangent, 2.5f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);

    // And a set of intersecting axes:
    origin[0] += 4.0f;
    origin[1] += 1.0f;
    DrawLabel(origin, "cross");
    GetDebugRender()->Cross(origin, 2.0f);
    GetDebugRender()->Point(origin, Color::White, 15.0f);
}

void DebugRenderDemo::DrawFrustum()
{
    // const ddVec3 color  = {  0.8f, 0.3f, 1.0f  };
    // const ddVec3 origin = { -8.0f, 0.5f, 14.0f };
    // drawLabel(ctx, origin, "frustum + axes");

    // // The frustum will depict a fake camera:
    // const Matrix4 proj = Matrix4::perspective(degToRad(45.0f), 800.0f / 600.0f, 0.5f, 4.0f);
    // const Matrix4 view = Matrix4::lookAt(Point3(-8.0f, 0.5f, 14.0f), Point3(-8.0f, 0.5f, -14.0f), Vector3::yAxis());
    // const Matrix4 clip = inverse(proj * view);
    // dd::frustum(ctx, toFloatPtr(clip), color);

    // // A white dot at the eye position:
    // dd::point(ctx, origin, dd::colors::White, 15.0f);

    // // A set of arrows at the camera's origin/eye:
    // const Matrix4 transform = Matrix4::translation(Vector3(-8.0f, 0.5f, 14.0f)) * Matrix4::rotationZ(degToRad(60.0f));
    // dd::axisTriad(ctx, toFloatPtr(transform), 0.3f, 2.0f);
}

void DebugRenderDemo::DrawText()
{
    // HUD text:
    // const ddVec3 textColor = { 1.0f,  1.0f,  1.0f };
    // const ddVec3 textPos2D = { 10.0f, 15.0f, 0.0f };
    // dd::screenText(ctx, "Welcome to the D3D11 Debug Draw demo.\n\n"
    //                     "[SPACE]  to toggle labels on/off\n"
    //                     "[RETURN] to toggle grid on/off",
    //                     textPos2D, textColor, 0.55f);
}

} // namespace WingsOfSteel::Pandora