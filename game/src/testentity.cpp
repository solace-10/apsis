#include "testentity.hpp"

namespace WingsOfSteel::TheBrightestStar
{

TestEntity::TestEntity()
{

}

TestEntity::~TestEntity()
{

}

void TestEntity::OnAddedToScene(Pandora::Scene* pScene)
{
    Pandora::Entity::OnAddedToScene(pScene);
}

void TestEntity::Update(float delta) 
{
    Pandora::Entity::Update(delta);
}

void TestEntity::Render()
{
    Pandora::Entity::Render();
}

} // namespace WingsOfSteel::TheBrightestStar