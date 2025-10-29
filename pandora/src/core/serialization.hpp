#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "core/smart_ptr.hpp"
#include "core/result.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ResourceDataStore);

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

} // namespace Json
} // namespace WingsOfSteel