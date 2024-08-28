#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::Forge
{

class Forge
{
public:
    Forge();
    ~Forge();

    void Initialize();
    void Update(float delta);
    void Shutdown();

private:
    void DrawImGuiMenuBar();
};

} // namespace WingsOfSteel::Forge