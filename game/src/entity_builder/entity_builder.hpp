#pragma once

#include <functional>
#include <string>

#include <glm/mat4x4.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource_data_store.hpp>
#include <scene/scene.hpp>
#include <scene/entity.hpp>

#include "components/hardpoint_component.hpp"

namespace WingsOfSteel
{

using OnEntityReady = std::function<void(EntitySharedPtr pEntity)>;

DECLARE_SMART_PTR(EntityBuilder);
class EntityBuilder
{
public:
    static void Build(SceneWeakPtr& pWeakScene, const std::string& resourcePath, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback);
    static void Build(const std::string& resourcePath, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback);

private:
    static void InstantiateComponents(EntitySharedPtr pEntity, const nlohmann::json& jsonData, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback, const std::string& prefabResourcePath);
};

} // namespace WingsOfSteel