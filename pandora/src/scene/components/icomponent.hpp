#pragma once

#include "core/serialization.hpp"
#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ResourceDataStore)

DECLARE_SMART_PTR(IComponent)
class IComponent
{
public:
    virtual ~IComponent() = default;

    virtual void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) = 0;
};

} // namespace WingsOfSteel
