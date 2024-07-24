#pragma once

#include <functional>
#include <memory>

namespace WingsOfSteel::Pandora
{

#define DECLARE_RESOURCE_TYPE(x) \
    class x; \
    using x ## SharedPtr = std::shared_ptr<x>; \
    using x ## WeakPtr = std::weak_ptr<x>

DECLARE_RESOURCE_TYPE(Resource);
DECLARE_RESOURCE_TYPE(ResourceModel);
DECLARE_RESOURCE_TYPE(ResourceShader);
DECLARE_RESOURCE_TYPE(ResourceTexture2D);

enum class ResourceState
{
    Unloaded,
    Loading,
    Loaded,
    Error
};

// Called by the Resource System when a resource has been instantiated and loaded.
// This is only called when the resource is fully available: failing to load a resource is a fatal error.
using OnResourceAvailableCallback = std::function<void(ResourceSharedPtr)>;

enum class ResourceType
{
    Invalid = 0,
    Shader,
    Texture2D,
    Model
};

} // namespace WingsOfSteel::Pandora