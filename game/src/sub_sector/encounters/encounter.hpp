#pragma once

#include <vector>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "encounter_blueprint_types.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Encounter);
class Encounter
{
public:
    Encounter(Pandora::ResourceDataStoreSharedPtr pDataStore);
    ~Encounter();

    void Update();

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& data);

    void AddNode(NodeUniquePtr pNode);
    const std::vector<Node*> GetNodes() const;

private:
    Pandora::ResourceDataStoreSharedPtr m_pDataStore;
    std::vector<NodeUniquePtr> m_Nodes;
};

inline void Encounter::AddNode(NodeUniquePtr pNode)
{
    m_Nodes.emplace_back(std::move(pNode));
}

} // namespace WingsOfSteel::TheBrightestStar