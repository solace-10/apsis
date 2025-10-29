#pragma once

#include <nlohmann/json.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

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

    virtual nlohmann::json Serialize() const = 0;
    virtual void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) = 0;

    // Helper functions for serialization (these are still needed for Serialize() methods)
    static nlohmann::json SerializeVec3(const glm::vec3& vec)
    {
        return nlohmann::json::array({vec.x, vec.y, vec.z});
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
};

} // namespace WingsOfSteel
