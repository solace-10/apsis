#pragma once

#include <entt/entt.hpp>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(Scene);

DECLARE_SMART_PTR(System);
class System
{
public:
    System() {}
    virtual ~System() {}

    virtual void Initialize(Scene* pScene) = 0;
    virtual void Update(float delta) = 0;
};

} // namespace WingsOfSteel::Pandora