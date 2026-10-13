#include "render/instance_parameter_buffer.hpp"

namespace WingsOfSteel
{

std::vector<DynamicUniformsData> InstanceParameterBuffer::PackParameters(
    const std::vector<ShaderParameterDefinition>& definitions,
    const std::vector<std::unordered_map<std::string, float>>& instanceParameters)
{
    std::vector<DynamicUniformsData> result;

    if (instanceParameters.empty())
    {
        return result;
    }

    result.resize(instanceParameters.size());

    // Pack each instance's parameters separately
    for (size_t instanceIdx = 0; instanceIdx < instanceParameters.size(); ++instanceIdx)
    {
        DynamicUniformsData& data = result[instanceIdx];
        const auto& params = instanceParameters[instanceIdx];

        // Initialize with default values
        for (const auto& def : definitions)
        {
            const size_t paramIndex = def.offset / 4;
            const size_t componentOffset = def.offset % 4;

            if (paramIndex >= DynamicUniformsData::MaxParams)
            {
                continue;
            }

            for (uint32_t i = 0; i < def.componentCount; i++)
            {
                const size_t finalParamIndex = (def.offset + i) / 4;
                const size_t finalComponentOffset = (def.offset + i) % 4;

                if (finalParamIndex < DynamicUniformsData::MaxParams)
                {
                    data.params[finalParamIndex][finalComponentOffset] = def.defaultValue[i];
                }
            }
        }

        // Override with instance-specific values
        for (const auto& def : definitions)
        {
            auto it = params.find(def.name);
            if (it != params.end())
            {
                const size_t paramIndex = def.offset / 4;
                const size_t componentOffset = def.offset % 4;

                if (paramIndex < DynamicUniformsData::MaxParams)
                {
                    data.params[paramIndex][componentOffset] = it->second;
                }
            }
        }
    }

    return result;
}

} // namespace WingsOfSteel
