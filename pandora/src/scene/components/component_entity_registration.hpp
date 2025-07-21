#pragma once

#include "component_factory.hpp"
#include "../entity.hpp"
#include <nlohmann/json.hpp>

namespace WingsOfSteel::Pandora
{

// This file provides the Entity registration functionality after Entity.hpp is available

template<typename ComponentType>
class ComponentEntityRegistrar
{
public:
    ComponentEntityRegistrar(const std::string& typeName)
    {
        ComponentFactory::RegisterEntityAdder<ComponentType>(typeName);
    }
};

#define REGISTER_COMPONENT_ENTITY_INTEGRATION(ComponentClass, TypeName) \
    static ComponentEntityRegistrar<ComponentClass> g_##ComponentClass##_entity_registrar(TypeName);

#define PANDORA_REGISTER_COMPONENT_ENTITY_INTEGRATION(ComponentClass, TypeName) \
    static WingsOfSteel::Pandora::ComponentEntityRegistrar<ComponentClass> g_##ComponentClass##_entity_registrar(TypeName);

} // namespace WingsOfSteel::Pandora