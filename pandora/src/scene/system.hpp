#pragma once

#include <entt/entt.hpp>

namespace WingsOfSteel::Pandora
{

class System
{
public:
    System() {}
    virtual ~System() {}

    virtual void Initialize() = 0;
    virtual void Update(float delta) = 0;
};

}