#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>

#include <nlohmann/json_fwd.hpp>

#include "icomponent.hpp"
#include "scene/entity.hpp"


namespace WingsOfSteel::Pandora
{
    class Entity;
}

namespace WingsOfSteel::Pandora
{

class ComponentFactory
{
private:
    using EntityAdderFunc = std::function<void(Entity*, const nlohmann::json&)>;
    
    inline static std::unordered_map<std::string, EntityAdderFunc>* s_entityAdders = nullptr;

public:    
    template<typename T>
    static void RegisterEntityAdder(const std::string& typeName)
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

    static bool CreateAndAddToEntity(Entity* pEntity, const std::string& typeName, const nlohmann::json& jsonData)
    {
        if (!s_entityAdders)
        {
            return false;
        }
        
        auto it = s_entityAdders->find(typeName);
        if (it != s_entityAdders->end())
        {
            it->second(pEntity, jsonData);
            return true;
        }
        return false;
    }

};

// Enhanced registrar that does both component creation and entity integration
// This version is safe to use in headers - entity registration happens when available
template<typename ComponentType>
class ComponentWithEntityRegistrar
{
public:
    ComponentWithEntityRegistrar(const std::string& typeName)
    {
        ComponentFactory::RegisterEntityAdder<ComponentType>(typeName);
    }
};

// Enhanced macro that registers both component creator and entity adder
// This is safe to use in headers - actual entity registration happens when Entity.hpp is available
#define REGISTER_COMPONENT(ComponentClass, TypeName) \
    static WingsOfSteel::Pandora::ComponentWithEntityRegistrar<ComponentClass> g_##ComponentClass##_full_registrar(TypeName);

} // namespace WingsOfSteel::Pandora