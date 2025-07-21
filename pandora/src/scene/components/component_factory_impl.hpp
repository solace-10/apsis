#pragma once

// This header provides the template implementation for ComponentFactory
// It can only be included in .cpp files where Entity.hpp is available

#include "component_factory.hpp"
#include "../entity.hpp"
#include <nlohmann/json.hpp>

namespace WingsOfSteel::Pandora
{

template<typename T>
void ComponentFactory::RegisterEntityAdder(const std::string& typeName)
{
    if (s_entityAdders == nullptr)
    {
        s_entityAdders = new std::unordered_map<std::string, EntityAdderFunc>();
    }

    (*s_entityAdders)[typeName] = [](Entity* pEntity, const nlohmann::json& jsonData) {
        T& component = pEntity->AddComponent<T>();
        component.Deserialize(jsonData);
    };
}

} // namespace WingsOfSteel::Pandora