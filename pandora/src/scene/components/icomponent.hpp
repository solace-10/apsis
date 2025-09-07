#pragma once

#include <nlohmann/json.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "core/log.hpp"
#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(IComponent)
class IComponent
{
public:
    virtual ~IComponent() = default;
    
    virtual nlohmann::json Serialize() const = 0;
    virtual void Deserialize(const nlohmann::json& json) = 0;

    // Helper functions for common serialization patterns
    static nlohmann::json SerializeVec3(const glm::vec3& vec)
    {
        return nlohmann::json::array({vec.x, vec.y, vec.z});
    }
    
    static glm::vec3 DeserializeVec3(const nlohmann::json& json, const std::string& fieldName)
    {
        if (!json.contains(fieldName))
        {
            Log::Error() << "Missing required field: " << fieldName;
            throw std::runtime_error("Missing required field: " + fieldName);
        }
        
        const auto& arr = json[fieldName];
        if (!arr.is_array() || arr.size() != 3)
        {
            Log::Error() << "Invalid vec3 format for field: " << fieldName;
            throw std::runtime_error("Invalid vec3 format for field: " + fieldName);
        }
        
        return glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
    }
    
    static nlohmann::json SerializeMat4(const glm::mat4& mat)
    {
        nlohmann::json result = nlohmann::json::array();
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                result.push_back(mat[i][j]);
            }
        }
        return result;
    }
    
    static glm::mat4 DeserializeMat4(const nlohmann::json& json, const std::string& fieldName)
    {
        if (!json.contains(fieldName))
        {
            Log::Error() << "Missing required field: " << fieldName;
            throw std::runtime_error("Missing required field: " + fieldName);
        }
        
        const auto& arr = json[fieldName];
        if (!arr.is_array() || arr.size() != 16)
        {
            Log::Error() << "Invalid mat4 format for field: " << fieldName;
            throw std::runtime_error("Invalid mat4 format for field: " + fieldName);
        }
        
        glm::mat4 result;
        int index = 0;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                result[i][j] = arr[index++].get<float>();
            }
        }
        return result;
    }
    
    template<typename T>
    static T DeserializeRequired(const nlohmann::json& json, const std::string& fieldName)
    {
        if (!json.contains(fieldName))
        {
            Log::Error() << "Missing required field: " << fieldName;
            throw std::runtime_error("Missing required field: " + fieldName);
        }
        
        try
        {
            return json[fieldName].get<T>();
        }
        catch (const std::exception& e)
        {
            Log::Error() << "Failed to deserialize field '" << fieldName << "': " << e.what();
            throw std::runtime_error("Failed to deserialize field '" + fieldName + "': " + e.what());
        }
    }
    
    template<typename T>
    static T DeserializeOptional(const nlohmann::json& json, const std::string& fieldName, const T& defaultValue)
    {
        if (!json.contains(fieldName))
        {
            return defaultValue;
        }
        
        try
        {
            return json[fieldName].get<T>();
        }
        catch (const std::exception& e)
        {
            Log::Error() << "Failed to deserialize optional field '" << fieldName << "', using default: " << e.what();
            return defaultValue;
        }
    }
    
    template<typename EnumType>
    static EnumType DeserializeEnum(const nlohmann::json& json, const std::string& fieldName, const EnumType& defaultValue)
    {
        if (!json.contains(fieldName))
        {
            Log::Error() << "Missing required enum field: " << fieldName;
            throw std::runtime_error("Missing required enum field: " + fieldName);
        }
        
        try
        {
            int enumValue = json[fieldName].get<int>();
            return static_cast<EnumType>(enumValue);
        }
        catch (const std::exception& e)
        {
            Log::Error() << "Failed to deserialize enum field '" << fieldName << "': " << e.what();
            throw std::runtime_error("Failed to deserialize enum field '" + fieldName + "': " + e.what());
        }
    }
    
    template<typename EnumType>
    static EnumType DeserializeEnumOptional(const nlohmann::json& json, const std::string& fieldName, const EnumType& defaultValue)
    {
        if (!json.contains(fieldName))
        {
            return defaultValue;
        }
        
        try
        {
            int enumValue = json[fieldName].get<int>();
            return static_cast<EnumType>(enumValue);
        }
        catch (const std::exception& e)
        {
            Log::Error() << "Failed to deserialize optional enum field '" << fieldName << "', using default: " << e.what();
            return defaultValue;
        }
    }
};

} // namespace WingsOfSteel