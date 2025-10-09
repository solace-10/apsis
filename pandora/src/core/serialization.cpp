#include "core/serialization.hpp"

#include "core/log.hpp"
#include "resources/resource_data_store.hpp"

namespace WingsOfSteel::Json
{

Result<DeserializationError, const Data> DeserializeArray(const ResourceDataStore* pContext, const Data& data, const std::string& key)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        Log::Warning() << pContext->GetPath() << ": failed to find key '" << key << "'.";
        return Result<DeserializationError, const Data>(DeserializationError::KeyNotFound);
    }
    else if (!it->is_array())
    {
        Log::Warning() << pContext->GetPath() << ": key '" << key << "' is not an array.";
        return Result<DeserializationError, const Data>(DeserializationError::TypeMismatch);
    }
    else
    {
        return Result<DeserializationError, const Data>(*it);
    }
}

Result<DeserializationError, const Data> DeserializeObject(const ResourceDataStore* pContext, const Data& data, const std::string& key)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        Log::Warning() << pContext->GetPath() << ": failed to find key '" << key << "'.";
        return Result<DeserializationError, const Data>(DeserializationError::KeyNotFound);
    }
    else if (!it->is_object())
    {
        Log::Warning() << pContext->GetPath() << ": key '" << key << "' is not an object.";
        return Result<DeserializationError, const Data>(DeserializationError::TypeMismatch);
    }
    else
    {
        return Result<DeserializationError, const Data>(*it);
    }
}

Result<DeserializationError, std::string> DeserializeString(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<std::string> defaultValue /* = std::nullopt */)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        if (defaultValue.has_value())
        {
            return Result<DeserializationError, std::string>(defaultValue.value());
        }
        Log::Warning() << pContext->GetPath() << ": failed to find key '" << key << "'.";
        return Result<DeserializationError, std::string>(DeserializationError::KeyNotFound);
    }
    else if (!it->is_string())
    {
        Log::Warning() << pContext->GetPath() << ": key '" << key << "' is not a string.";
        return Result<DeserializationError, std::string>(DeserializationError::TypeMismatch);
    }
    else
    {
        const std::string value = it->get<std::string>();
        return Result<DeserializationError, std::string>(value);
    }
}

Result<DeserializationError, uint32_t> DeserializeUnsignedInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<uint32_t> defaultValue /* = std::nullopt */)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        if (defaultValue.has_value())
        {
            return Result<DeserializationError, uint32_t>(defaultValue.value());
        }
        Log::Warning() << pContext->GetPath() << ": failed to find key '" << key << "'.";
        return Result<DeserializationError, uint32_t>(DeserializationError::KeyNotFound);
    }
    else if (!it->is_number_unsigned())
    {
        Log::Warning() << pContext->GetPath() << ": key '" << key << "' is not an unsigned integer.";
        return Result<DeserializationError, uint32_t>(DeserializationError::TypeMismatch);
    }
    else
    {
        const uint32_t value = it->get<uint32_t>();
        return Result<DeserializationError, uint32_t>(value);
    }
}

} // namespace WingsOfSteel