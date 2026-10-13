#pragma once

#include <glm/glm.hpp>

#include "imgui/idebugui.hpp"
#include "scene/entity.hpp"
#include "scene/systems/system.hpp"
#include "scene/components/landscape_component.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Scene);

class LandscapeSystem : public System, public IDebugUI
{
public:
    LandscapeSystem();
    ~LandscapeSystem();

    void Initialize(Scene* pScene) override{};
    void Update(float delta) override;

    void Generate(EntitySharedPtr pLandscapeEntity);

    void DrawDebugUI() override;

private:
    void GenerateInternal(LandscapeComponent& landscapeComponent);
    glm::vec2 NormalizedCoords(const LandscapeComponent& landscapeComponent, int x, int y) const;
    void GenerateDebugHeightmapTexture(LandscapeComponent& landscapeComponent);
    void BuildPhysics(EntitySharedPtr pLandscapeEntity, const LandscapeComponent& landscapeComponent);
    EntityWeakPtr m_pLandscapeEntity;
};

} // namespace WingsOfSteel
