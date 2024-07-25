#pragma once

#include <glm/vec3.hpp>

#include <string>

namespace WingsOfSteel::Pandora
{

class DebugRenderDemo
{
public:
    DebugRenderDemo();
    ~DebugRenderDemo();

    void Show(bool state);
    void Update(float delta);

private:
    void DrawGrid();
    void DrawLabel(const glm::vec3& pos, const std::string& name);
    void DrawMiscObjects();
    void DrawFrustum();
    void DrawText();

    bool m_Show;
};

} // namespace WingsOfSteel::Pandora