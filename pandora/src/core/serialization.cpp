#include "core/serialization.hpp"

#include <magic_enum.hpp>

#include "core/log.hpp"
#include "resources/resource_data_store.hpp"

namespace WingsOfSteel::Json
{

void DefaultErrorHandler(const ResourceDataStore* pContext, const std::string& key, DeserializationError error, const std::string& expectedType)
{
    if (error == DeserializationError::KeyNotFound)
    {
        Log::Error() << pContext->GetPath() << ": failed to find key '" << key << "'.";
    }
    else if (error == DeserializationError::TypeMismatch)
    {
        Log::Error() << pContext->GetPath() << ": key '" << key << "' is not '" << expectedType << "'.";
    }
    else
    {
        Log::Error() << pContext->GetPath() << ": unhandled deserialization error '" << magic_enum::enum_name(error) << "'.";
    }
}

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

std::string DeserializeString(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<std::string> defaultValue /* = std::nullopt */)
{
    auto result = TryDeserializeString(pContext, data, key, defaultValue);
    if (result.has_value())
    {
        return result.value();
    }
    else
    {
        DefaultErrorHandler(pContext, key, result.error(), "string");
        return "";
    }
}

Result<DeserializationError, std::string> TryDeserializeString(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<std::string> defaultValue /* = std::nullopt */)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        if (defaultValue.has_value())
        {
            return Result<DeserializationError, std::string>(defaultValue.value());
        }
        else
        {
            return Result<DeserializationError, std::string>(DeserializationError::KeyNotFound);
        }
    }
    else if (!it->is_string())
    {
        return Result<DeserializationError, std::string>(DeserializationError::TypeMismatch);
    }
    else
    {
        const std::string value = it->get<std::string>();
        return Result<DeserializationError, std::string>(value);
    }
}

uint32_t DeserializeUnsignedInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<uint32_t> defaultValue /* = std::nullopt */)
{
    auto result = TryDeserializeUnsignedInteger(pContext, data, key, defaultValue);
    if (result.has_value())
    {
        return result.value();
    }
    else
    {
        DefaultErrorHandler(pContext, key, result.error(), "unsigned integer");
        return 0;
    }
}

Result<DeserializationError, uint32_t> TryDeserializeUnsignedInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<uint32_t> defaultValue /* = std::nullopt */)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        if (defaultValue.has_value())
        {
            return Result<DeserializationError, uint32_t>(defaultValue.value());
        }
        else
        {
            return Result<DeserializationError, uint32_t>(DeserializationError::KeyNotFound);
        }
    }
    else if (!it->is_number_unsigned())
    {
        return Result<DeserializationError, uint32_t>(DeserializationError::TypeMismatch);
    }
    else
    {
        const uint32_t value = it->get<uint32_t>();
        return Result<DeserializationError, uint32_t>(value);
    }
}

float DeserializeFloat(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<float> defaultValue /* = std::nullopt */)
{
    auto result = TryDeserializeFloat(pContext, data, key, defaultValue);
    if (result.has_value())
    {
        return result.value();
    }
    else
    {
        DefaultErrorHandler(pContext, key, result.error(), "float");
        return 0.0f;
    }
}

Result<DeserializationError, float> TryDeserializeFloat(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<float> defaultValue /* = std::nullopt */)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        if (defaultValue.has_value())
        {
            return Result<DeserializationError, float>(defaultValue.value());
        }
        else
        {
            return Result<DeserializationError, float>(DeserializationError::KeyNotFound);
        }
    }
    else if (!it->is_number())
    {
        return Result<DeserializationError, float>(DeserializationError::TypeMismatch);
    }
    else
    {
        const float value = it->get<float>();
        return Result<DeserializationError, float>(value);
    }
}

bool DeserializeBool(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<bool> defaultValue /* = std::nullopt */)
{
    auto result = TryDeserializeBool(pContext, data, key, defaultValue);
    if (result.has_value())
    {
        return result.value();
    }
    else
    {
        DefaultErrorHandler(pContext, key, result.error(), "bool");
        return false;
    }
}

Result<DeserializationError, bool> TryDeserializeBool(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<bool> defaultValue /* = std::nullopt */)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        if (defaultValue.has_value())
        {
            return Result<DeserializationError, bool>(defaultValue.value());
        }
        else
        {
            return Result<DeserializationError, bool>(DeserializationError::KeyNotFound);
        }
    }
    else if (!it->is_boolean())
    {
        return Result<DeserializationError, bool>(DeserializationError::TypeMismatch);
    }
    else
    {
        const bool value = it->get<bool>();
        return Result<DeserializationError, bool>(value);
    }
}

} // namespace WingsOfSteel