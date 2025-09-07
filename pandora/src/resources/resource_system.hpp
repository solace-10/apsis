#pragma once

#include <atomic>
#include <functional>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/signal.hpp"
#include "resources/resource.fwd.hpp"

namespace WingsOfSteel::Pandora
{

using ShaderInjectedSignal = Signal<ResourceShader*>;

class ResourceSystem
{
public:
    ResourceSystem();
    ~ResourceSystem();
    void Update();

    void RequestResource(const std::string& path, OnResourceAvailableCallback onResourceLoaded);
    void RequestResources(const std::vector<std::string>& paths, OnResourcesAvailableCallback onResourcesLoaded);

    ShaderInjectedSignal& GetShaderInjectedSignal();

private:
    template <typename T>
    void RegisterResource(const std::string& extension)
    {
        m_ResourceCreationFunctions[extension] = []() {
            return std::make_shared<T>();
        };
    }

    std::optional<std::string> GetExtension(const std::string& path) const;

    std::unordered_map<std::string, ResourceSharedPtr> m_Resources;

    using ResourceCreationFunction = std::function<ResourceSharedPtr()>;
    std::unordered_map<std::string, ResourceCreationFunction> m_ResourceCreationFunctions;

    struct PendingResource
    {
        ResourceSharedPtr pResource;
        OnResourceAvailableCallback onResourceAvailable;
    };
    std::list<PendingResource> m_PendingResources;

    using MultiPendingResourceHandle = uint32_t;
    struct MultiPendingResource
    {
        MultiPendingResource()
            : pending(0)
        {
        }

        MultiPendingResource(size_t numPending, OnResourcesAvailableCallback callback)
            : pending(numPending)
            , onResourcesAvailable(callback)
        {
            resources.reserve(numPending);
        }

        MultiPendingResource(const MultiPendingResource& other)
            : pending(other.pending.load())
            , resources(other.resources)
            , onResourcesAvailable(other.onResourcesAvailable)
        {
        }

        MultiPendingResource& operator=(const MultiPendingResource& other)
        {
            pending.store(other.pending.load());
            resources = other.resources;
            onResourcesAvailable = other.onResourcesAvailable;
            return *this;
        }

        MultiPendingResource(MultiPendingResource&& other)
            : pending(other.pending.load())
            , resources(std::move(other.resources))
            , onResourcesAvailable(std::move(other.onResourcesAvailable))
        {
        }

        std::atomic<size_t> pending{ 0 };
        std::vector<ResourceSharedPtr> resources;
        OnResourcesAvailableCallback onResourcesAvailable;
    };
    std::unordered_map<MultiPendingResourceHandle, MultiPendingResource> m_MultiRequests;
    MultiPendingResourceHandle m_NextMultiRequestHandle{ 0 };

    ShaderInjectedSignal m_ShaderInjectedSignal;
};

} // namespace WingsOfSteel::Pandora