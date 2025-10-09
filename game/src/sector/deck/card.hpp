#pragma once

#include <string>

#include <core/serialization.hpp>
#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Entity);

DECLARE_SMART_PTR(Card);
class Card
{
public:
    enum class Type
    {
        DeployEscort
    };

    Card() = default;
    virtual ~Card() = default;

    virtual bool Deserialize(const ResourceDataStore* pContext, const Json::Data& data) = 0;
    virtual Type GetType() const = 0;
    virtual const std::string& GetName() const = 0;
    virtual void Play() = 0;
};

} // namespace WingsOfSteel