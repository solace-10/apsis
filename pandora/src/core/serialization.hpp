#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

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
Result<DeserializationError, std::string> DeserializeString(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<std::string> defaultValue = std::nullopt);
Result<DeserializationError, uint32_t> DeserializeUnsignedInteger(const ResourceDataStore* pContext, const Data& data, const std::string& key, std::optional<uint32_t> defaultValue = std::nullopt);

} // namespace Json
} // namespace WingsOfSteel