#include <core/log.hpp>
#include <resources/resource_data_store.hpp>
#include <pandora.hpp>

#include "encounter.hpp"
#include "encounter_blueprint_nodes.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Encounter::Encounter(Pandora::ResourceDataStoreSharedPtr pDataStore)
: m_pDataStore(pDataStore)
{
    Load();
}

Encounter::~Encounter()
{
}

void Encounter::Update()
{
}

void Encounter::Save()
{
    nlohmann::json data;
    data["nodes"] = nlohmann::json::array();
    for (auto& pNode : m_Nodes)
    {
        data["nodes"].push_back(pNode->Serialize());
    }
    
    data["links"] = nlohmann::json::array();
    m_pDataStore->Inject(data);
}

void Encounter::Revert()
{
    m_Nodes.clear();
    Load();
}

void Encounter::Load()
{
    const nlohmann::json& encounterData = m_pDataStore->Data();
    const auto nodesIt = encounterData.find("nodes");
    if (nodesIt != encounterData.end() && nodesIt->is_array())
    {
        for (const auto& nodeData : *nodesIt)
        {
            const auto nodeNameIt = nodeData.find("name");
            if (nodeNameIt != nodeData.end() && nodeNameIt->is_string())
            {
                const std::string nodeName = nodeNameIt->get<std::string>();
                NodeUniquePtr pNode = BlueprintNodeFactory::CreateNode(nodeName);
                if (pNode)
                {
                    pNode->Deserialize(nodeData);
                    m_Nodes.emplace_back(std::move(pNode));
                }
                else
                {
                    Pandora::Log::Warning() << "Unknown node type: " << nodeName;
                }
            }
        }
    }
}

const std::vector<Node*> Encounter::GetNodes() const
{
    std::vector<Node*> nodes;
    for (const auto& node : m_Nodes)
    {
        nodes.push_back(node.get());
    }
    return nodes;
}

} // namespace WingsOfSteel::TheBrightestStar
