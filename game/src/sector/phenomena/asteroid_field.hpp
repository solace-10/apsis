#pragma once

#include <webgpu/webgpu_cpp.h>

#include <scene/entity.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(AsteroidFieldSignal);

DECLARE_SMART_PTR(AsteroidField);
class AsteroidField : public Pandora::Entity
{
public:
    AsteroidField(const AsteroidFieldSignal* pSignal);
    ~AsteroidField();

    void OnAddedToScene(Pandora::Scene* pScene) override;
    void Update(float delta) override;
    void Render(wgpu::RenderPassEncoder renderPass) override;

private:
    const AsteroidFieldSignal* m_pSignal;
};

} // namespace WingsOfSteel::TheBrightestStar