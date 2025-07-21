#pragma once

#include <optional>

#include "core/color.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel::Pandora
{

enum class DebugRenderShape
{
    Circle,
    Cone,
    Box,
    Sphere
};

class DebugRenderComponent : public IComponent
{
public:
    DebugRenderComponent() {}

    std::optional<float> radius;
    std::optional<float> width;
    std::optional<float> height;
    std::optional<float> length;
    Color color = Color::White;
    DebugRenderShape shape = DebugRenderShape::Cone;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        
        if (radius.has_value())
        {
            json["radius"] = radius.value();
            json["hasRadius"] = true;
        }
        else
        {
            json["hasRadius"] = false;
        }
        
        if (width.has_value())
        {
            json["width"] = width.value();
            json["hasWidth"] = true;
        }
        else
        {
            json["hasWidth"] = false;
        }
        
        if (height.has_value())
        {
            json["height"] = height.value();
            json["hasHeight"] = true;
        }
        else
        {
            json["hasHeight"] = false;
        }
        
        if (length.has_value())
        {
            json["length"] = length.value();
            json["hasLength"] = true;
        }
        else
        {
            json["hasLength"] = false;
        }
        
        json["color"] = nlohmann::json::array({color.r, color.g, color.b});
        json["shape"] = static_cast<int>(shape);
        
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        bool hasRadius = DeserializeOptional<bool>(json, "hasRadius", false);
        if (hasRadius)
        {
            radius = DeserializeRequired<float>(json, "radius");
        }
        else
        {
            radius.reset();
        }
        
        bool hasWidth = DeserializeOptional<bool>(json, "hasWidth", false);
        if (hasWidth)
        {
            width = DeserializeRequired<float>(json, "width");
        }
        else
        {
            width.reset();
        }
        
        bool hasHeight = DeserializeOptional<bool>(json, "hasHeight", false);
        if (hasHeight)
        {
            height = DeserializeRequired<float>(json, "height");
        }
        else
        {
            height.reset();
        }
        
        bool hasLength = DeserializeOptional<bool>(json, "hasLength", false);
        if (hasLength)
        {
            length = DeserializeRequired<float>(json, "length");
        }
        else
        {
            length.reset();
        }
        
        const auto& colorArray = json["color"];
        if (!colorArray.is_array() || colorArray.size() != 3)
        {
            Pandora::Log::Error() << "Invalid color format";
            throw std::runtime_error("Invalid color format");
        }
        color = Color(colorArray[0].get<float>(), colorArray[1].get<float>(), 
                     colorArray[2].get<float>());
        
        shape = DeserializeEnum<DebugRenderShape>(json, "shape", DebugRenderShape::Cone);
    }
};

REGISTER_COMPONENT(DebugRenderComponent, "debug_render")

} // namespace WingsOfSteel::Pandora