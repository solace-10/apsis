#pragma once

#include <optional>

#include <resources/resource_data_store.hpp>

#include "components/metadata_component.hpp"
#include "components/orbital_elements_component.hpp"

namespace WingsOfSteel
{

namespace OMMDeserializer
{

struct DeserializedData
{
    OrbitalElementsComponent orbitalElements;
    MetadataComponent metadata;
};

std::optional<DeserializedData> Deserialize(const Json::Data& data);

} // namespace OMMDeserializer

} // namespace WingsOfSteel
