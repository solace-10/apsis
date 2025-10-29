#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <magic_enum.hpp>

#include "core/log.hpp"
#include "core/smart_ptr.hpp"
#include "core/result.hpp"
#include "resources/resource_data_store.hpp"

namespace WingsOfSteel
{

namespace Json
{

using Data = nlohmann::json;

enum class DeserializationError
{
    KeyNotFound,
    TypeMismatch
};

Result<DeserializationError, const Data> DeserializeArray(const ResourceDataStore* pContext, const Data& data, const std::string& key);
Result<DeserializationError, const Data> DeserializeObject(const ResourceDataStore* pContext, const Data& data, const std::string& key);

std::string DeserializeString(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<std::string> defaultValue = std::nullopt);
Result<DeserializationError, std::string> TryDeserializeString(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<std::string> defaultValue = std::nullopt);

uint32_t DeserializeUnsignedInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<uint32_t> defaultValue = std::nullopt);
Result<DeserializationError, uint32_t> TryDeserializeUnsignedInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<uint32_t> defaultValue = std::nullopt);

int32_t DeserializeInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<int32_t> defaultValue = std::nullopt);
Result<DeserializationError, int32_t> TryDeserializeInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<int32_t> defaultValue = std::nullopt);

float DeserializeFloat(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<float> defaultValue = std::nullopt);
Result<DeserializationError, float> TryDeserializeFloat(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<float> defaultValue = std::nullopt);

bool DeserializeBool(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<bool> defaultValue = std::nullopt);
Result<DeserializationError, bool> TryDeserializeBool(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<bool> defaultValue = std::nullopt);

glm::vec2 DeserializeVec2(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::vec2> defaultValue = std::nullopt);
Result<DeserializationError, glm::vec2> TryDeserializeVec2(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::vec2> defaultValue = std::nullopt);

glm::vec3 DeserializeVec3(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::vec3> defaultValue = std::nullopt);
Result<DeserializationError, glm::vec3> TryDeserializeVec3(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::vec3> defaultValue = std::nullopt);

glm::vec4 DeserializeVec4(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::vec4> defaultValue = std::nullopt);
Result<DeserializationError, glm::vec4> TryDeserializeVec4(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::vec4> defaultValue = std::nullopt);

glm::mat4 DeserializeMat4(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::mat4> defaultValue = std::nullopt);
Result<DeserializationError, glm::mat4> TryDeserializeMat4(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<glm::mat4> defaultValue = std::nullopt);

// Enum deserialization (template functions must be defined in header)
// Uses magic_enum to convert string values to enum types
template<typename EnumType>
EnumType DeserializeEnum(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<EnumType> defaultValue = std::nullopt)
{
    auto result = TryDeserializeEnum<EnumType>(pContext, data, key, defaultValue);
    if (result.has_value())
    {
        return result.value();
    }
    else
    {
        const std::string contextPath = pContext ? pContext->GetPath() : "<no context>";

        if (result.error() == DeserializationError::KeyNotFound)
        {
            Log::Error() << contextPath << ": failed to find enum key '" << key << "'.";
        }
        else if (result.error() == DeserializationError::TypeMismatch)
        {
            Log::Error() << contextPath << ": key '" << key << "' is not a valid enum string.";
        }

        return defaultValue.value_or(static_cast<EnumType>(0));
    }
}

template<typename EnumType>
Result<DeserializationError, EnumType> TryDeserializeEnum(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<EnumType> defaultValue = std::nullopt)
{
    auto it = data.find(key);
    if (it == data.cend())
    {
        if (defaultValue.has_value())
        {
            return Result<DeserializationError, EnumType>(defaultValue.value());
        }
        else
        {
            return Result<DeserializationError, EnumType>(DeserializationError::KeyNotFound);
        }
    }
    else if (!it->is_string())
    {
        return Result<DeserializationError, EnumType>(DeserializationError::TypeMismatch);
    }
    else
    {
        const std::string enumString = it->get<std::string>();
        auto enumValue = magic_enum::enum_cast<EnumType>(enumString, magic_enum::case_insensitive);

        if (enumValue.has_value())
        {
            return Result<DeserializationError, EnumType>(enumValue.value());
        }
        else
        {
            // Invalid enum string - log a warning and use default if available
            const std::string contextPath = pContext ? pContext->GetPath() : "<no context>";
            Log::Warning() << contextPath << ": invalid enum value '" << enumString << "' for key '" << key << "'.";

            if (defaultValue.has_value())
            {
                return Result<DeserializationError, EnumType>(defaultValue.value());
            }
            else
            {
                return Result<DeserializationError, EnumType>(DeserializationError::TypeMismatch);
            }
        }
    }
}

} // namespace Json
} // namespace WingsOfSteel