#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "render/material.hpp"

namespace WingsOfSteel
{

class InstanceParameterBuffer
{
public:
    InstanceParameterBuffer() = default;
    ~InstanceParameterBuffer() = default;

    // Pack parameters for all instances (returns vector with one entry per instance)
    static std::vector<DynamicUniformsData> PackParameters(
        const std::vector<ShaderParameterDefinition>& definitions,
        const std::vector<std::unordered_map<std::string, float>>& instanceParameters);
};

} // namespace WingsOfSteel
