#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include "icomponent.hpp"

// Forward declarations to avoid circular dependencies
#include <nlohmann/json_fwd.hpp>

namespace WingsOfSteel::Pandora
{
    class Entity;
}

namespace WingsOfSteel::Pandora
{

class ComponentFactory
{
private:
    using CreateFunc = std::function<std::unique_ptr<IComponent>()>;
    using EntityAdderFunc = std::function<void(Entity*, const nlohmann::json&)>;
    
    static std::unordered_map<std::string, CreateFunc>* s_creators;
    static std::unordered_map<std::string, EntityAdderFunc>* s_entityAdders;

public:
    template<typename T>
    static void Register(const std::string& typeName)
    {
        static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

        // Initialize on first use, as the order of static initialization is undefined.
        if (s_creators == nullptr)
        {
            s_creators = new std::unordered_map<std::string, CreateFunc>();
        }

        (*s_creators)[typeName] = []() { return std::make_unique<T>(); };
    }
    
    template<typename T>
    static void RegisterEntityAdder(const std::string& typeName);
    
    static std::unique_ptr<IComponent> Create(const std::string& typeName)
    {
        auto it = s_creators->find(typeName);
        if (it != s_creators->end())
        {
            return it->second();
        }
        return nullptr;
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
    
    static bool IsRegistered(const std::string& typeName)
    {
        return s_creators && s_creators->find(typeName) != s_creators->end();
    }
    
    static std::vector<std::string> GetRegisteredTypes()
    {
        std::vector<std::string> types;
        types.reserve(s_creators->size());
        for (const auto& pair : *s_creators)
        {
            types.push_back(pair.first);
        }
        return types;
    }

};

template<typename ComponentType>
class ComponentRegistrar
{
public:
    ComponentRegistrar(const std::string& typeName)
    {
        ComponentFactory::Register<ComponentType>(typeName);
    }
};

#define REGISTER_COMPONENT(ComponentClass, TypeName) \
    static ComponentRegistrar<ComponentClass> g_##ComponentClass##_registrar(TypeName);

#define PANDORA_REGISTER_COMPONENT(ComponentClass, TypeName) \
    static WingsOfSteel::Pandora::ComponentRegistrar<ComponentClass> g_##ComponentClass##_registrar(TypeName);

} // namespace WingsOfSteel::Pandora