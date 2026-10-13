#pragma once

#include <optional>

#include "core/color.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"

namespace WingsOfSteel
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

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        bool hasRadius = Json::DeserializeBool(pContext, json, "has_radius", false);
        if (hasRadius)
        {
            radius = Json::DeserializeFloat(pContext, json, "radius");
        }
        else
        {
            radius.reset();
        }

        bool hasWidth = Json::DeserializeBool(pContext, json, "has_width", false);
        if (hasWidth)
        {
            width = Json::DeserializeFloat(pContext, json, "width");
        }
        else
        {
            width.reset();
        }

        bool hasHeight = Json::DeserializeBool(pContext, json, "has_height", false);
        if (hasHeight)
        {
            height = Json::DeserializeFloat(pContext, json, "height");
        }
        else
        {
            height.reset();
        }

        bool hasLength = Json::DeserializeBool(pContext, json, "has_length", false);
        if (hasLength)
        {
            length = Json::DeserializeFloat(pContext, json, "length");
        }
        else
        {
            length.reset();
        }

        const auto& colorArray = json["color"];
        if (!colorArray.is_array() || colorArray.size() != 3)
        {
            Log::Error() << "Invalid color format";
            throw std::runtime_error("Invalid color format");
        }
        color = Color(colorArray[0].get<float>(), colorArray[1].get<float>(),
                     colorArray[2].get<float>());

        shape = Json::DeserializeEnum<DebugRenderShape>(pContext, json, "shape", DebugRenderShape::Cone);
    }
};

REGISTER_COMPONENT(DebugRenderComponent, "debug_render")

} // namespace WingsOfSteel