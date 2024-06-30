#pragma once

#include "resources/resource_shader.hpp"
#include "scene/entity.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(TestEntity);
class TestEntity : public Pandora::Entity
{
public:
    TestEntity();
    ~TestEntity();

    void OnAddedToScene(Pandora::Scene* pScene) override;
    void Update(float delta) override;
    void Render() override;

private:

};

} // namespace WingsOfSteel::TheBrightestStar