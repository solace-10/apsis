#pragma once

#include <unordered_map>
#include <functional>
#include <string>

#include <nlohmann/json_fwd.hpp>

#include "core/serialization.hpp"
#include "icomponent.hpp"
#include "scene/entity.hpp"

namespace WingsOfSteel
{

class ComponentFactory
{
private:
    using EntityAdderFunc = std::function<void(Entity*, const ResourceDataStore*, const Json::Data&)>;

    inline static std::unordered_map<std::string, EntityAdderFunc>* sRegistry = nullptr;

public:
    template<typename T>
    static void Register(const std::string& typeName)
    {
        if (sRegistry == nullptr)
        {
            sRegistry = new std::unordered_map<std::string, EntityAdderFunc>();
        }

        (*sRegistry)[typeName] = [](Entity* pEntity, const ResourceDataStore* pContext, const Json::Data& jsonData) {
            T& component = pEntity->AddComponent<T>();
            component.Deserialize(pContext, jsonData);
        };
    }

    static bool Create(Entity* pEntity, const ResourceDataStore* pContext, const std::string& typeName, const Json::Data& jsonData)
    {
        if (!sRegistry)
        {
            return false;
        }

        auto it = sRegistry->find(typeName);
        if (it != sRegistry->cend())
        {
            it->second(pEntity, pContext, jsonData);
            return true;
        }

        return false;
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
    class ComponentClass; \
    static WingsOfSteel::ComponentRegistrar<ComponentClass> g_##ComponentClass##_registrar(TypeName);

} // namespace WingsOfSteel