#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include "icomponent.hpp"

namespace WingsOfSteel::Pandora
{

class ComponentFactory
{
private:
    using CreateFunc = std::function<std::unique_ptr<IComponent>()>;
    static std::unordered_map<std::string, CreateFunc>* s_creators;

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
    
    static std::unique_ptr<IComponent> Create(const std::string& typeName)
    {
        auto it = s_creators->find(typeName);
        if (it != s_creators->end())
        {
            return it->second();
        }
        return nullptr;
    }
    
    static bool IsRegistered(const std::string& typeName)
    {
        return s_creators->find(typeName) != s_creators->end();
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