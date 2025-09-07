#pragma once

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Manifest);

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
    void DrawManifestTab();

    ManifestUniquePtr m_pManifest;
};

} // namespace WingsOfSteel