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
            json["has_radius"] = true;
        }
        else
        {
            json["has_radius"] = false;
        }
        
        if (width.has_value())
        {
            json["width"] = width.value();
            json["has_width"] = true;
        }
        else
        {
            json["has_width"] = false;
        }
        
        if (height.has_value())
        {
            json["height"] = height.value();
            json["has_height"] = true;
        }
        else
        {
            json["has_height"] = false;
        }
        
        if (length.has_value())
        {
            json["length"] = length.value();
            json["has_length"] = true;
        }
        else
        {
            json["has_length"] = false;
        }
        
        json["color"] = nlohmann::json::array({color.r, color.g, color.b});
        json["shape"] = static_cast<int>(shape);
        
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        bool hasRadius = DeserializeOptional<bool>(json, "has_radius", false);
        if (hasRadius)
        {
            radius = DeserializeRequired<float>(json, "radius");
        }
        else
        {
            radius.reset();
        }
        
        bool hasWidth = DeserializeOptional<bool>(json, "has_width", false);
        if (hasWidth)
        {
            width = DeserializeRequired<float>(json, "width");
        }
        else
        {
            width.reset();
        }
        
        bool hasHeight = DeserializeOptional<bool>(json, "has_height", false);
        if (hasHeight)
        {
            height = DeserializeRequired<float>(json, "height");
        }
        else
        {
            height.reset();
        }
        
        bool hasLength = DeserializeOptional<bool>(json, "has_length", false);
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