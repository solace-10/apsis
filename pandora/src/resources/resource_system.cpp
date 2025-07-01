#include "resources/resource_system.hpp"

#include "core/log.hpp"
#include "resources/resource.hpp"
#include "resources/resource_data_store.hpp"
#include "resources/resource_font.hpp"
#include "resources/resource_model.hpp"
#include "resources/resource_shader.hpp"
#include "resources/resource_texture_2d.hpp"

namespace WingsOfSteel::Pandora
{

ResourceSystem::ResourceSystem()
{
    RegisterResource<ResourceDataStore>("json");
    RegisterResource<ResourceFont>("ttf");
    RegisterResource<ResourceModel>("glb");
    RegisterResource<ResourceModel>("gltf");
    RegisterResource<ResourceShader>("wgsl");
    RegisterResource<ResourceTexture2D>("jpg");
    RegisterResource<ResourceTexture2D>("png");
}

ResourceSystem::~ResourceSystem()
{
}

void ResourceSystem::Update()
{
    for (auto& pPendingResource : m_PendingResources)
    {
        if (pPendingResource.pResource->GetState() == ResourceState::Error)
        {
            Log::Error() << "Failed to load resource '" << pPendingResource.pResource->GetPath() << "'.";
            exit(-1);
        }
        else if (pPendingResource.pResource->GetState() == ResourceState::Loaded)
        {
            Log::Info() << "Loaded resource '" << pPendingResource.pResource->GetPath() << "'.";
            pPendingResource.onResourceAvailable(pPendingResource.pResource);
        }
    }

    m_PendingResources.remove_if([](const PendingResource& pendingResource) {
        return pendingResource.pResource->GetState() == ResourceState::Loaded;
    });
}

void ResourceSystem::RequestResource(const std::string& path, OnResourceAvailableCallback onResourceAvailable)
{
    auto resourceIt = m_Resources.find(path);
    if (resourceIt != m_Resources.end())
    {
        ResourceSharedPtr pResource = resourceIt->second;

        if (pResource->GetState() == ResourceState::Loaded)
        {
            // If the resource has already completed loading, immediately trigger the callback.
            onResourceAvailable(resourceIt->second);
        }
        else if (pResource->GetState() == ResourceState::Loading)
        {
            // If we are still loading, add this request to the list of pending requests.
            // When the resource is loaded, all the callbacks will be triggered.
            m_PendingResources.push_back(
                PendingResource{
                    .pResource = pResource,
                    .onResourceAvailable = onResourceAvailable });
        }
        else
        {
            Log::Error() << "Invalid state for resource.";
        }
        return;
    }

    std::optional<std::string> extension = GetExtension(path);
    if (!extension.has_value())
    {
        Log::Error() << "ResourceSystem: requested path '" << path << "' has no extension.";
        return;
    }

    auto resourceCreatorIt = m_ResourceCreationFunctions.find(extension.value());
    if (resourceCreatorIt == m_ResourceCreationFunctions.end())
    {
        Log::Error() << "ResourceSystem: don't know how to create resource for '" << path << "'.";
        return;
    }

    Log::Info() << "ResourceSystem: requesting load for '" << path << "'.";

    ResourceSharedPtr pResource = resourceCreatorIt->second();
    m_Resources[path] = pResource;
    pResource->Load(path);

    m_PendingResources.push_back(
        PendingResource{
            .pResource = pResource,
            .onResourceAvailable = onResourceAvailable });
}

void ResourceSystem::RequestResources(const std::vector<std::string>& paths, OnResourcesAvailableCallback onResourcesLoaded)
{
    MultiPendingResourceHandle handle = m_NextMultiRequestHandle++;
    m_MultiRequests[handle] = MultiPendingResource(paths.size(), onResourcesLoaded);

    for (const auto& path : paths)
    {
        RequestResource(path, [this, handle, path](ResourceSharedPtr pResource) {
            auto& multiRequest = m_MultiRequests[handle];
            multiRequest.resources.push_back(pResource);

            if (--multiRequest.pending == 0)
            {
                std::unordered_map<std::string, ResourceSharedPtr> resources;
                for (const auto& resource : multiRequest.resources)
                {
                    resources[resource->GetPath()] = resource;
                }
                multiRequest.onResourcesAvailable(resources);
                m_MultiRequests.erase(handle);
            }
        });
    }
}

ShaderInjectedSignal& ResourceSystem::GetShaderInjectedSignal()
{
    return m_ShaderInjectedSignal;
}

std::optional<std::string> ResourceSystem::GetExtension(const std::string& path) const
{
    size_t separator = path.find_last_of('.');
    if (separator == std::string::npos || separator == path.length())
    {
        return nullptr;
    }
    else
    {
        return path.substr(separator + 1);
    }
}

} // namespace WingsOfSteel::Pandora