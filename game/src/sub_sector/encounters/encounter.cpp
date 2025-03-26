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
    for (auto& pLink : m_Links)
    {
        nlohmann::json link
        {
            {"id", pLink->ID.Get()},
            {"start", pLink->StartPinID.Get()},
            {"end", pLink->EndPinID.Get()}
        };
        data["links"].push_back(link);
    }
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
                    AddNode(std::move(pNode));
                }
                else
                {
                    Pandora::Log::Warning() << "Unknown node type: " << nodeName;
                }
            }
        }
    }

    const auto linksIt = encounterData.find("links");
    if (linksIt != encounterData.end() && linksIt->is_array())
    {
        for (const auto& linkData : *linksIt)
        {
            const auto linkIdIt = linkData.find("id");
            const auto linkStartIt = linkData.find("start");
            const auto linkEndIt = linkData.find("end");
            if (linkIdIt != linkData.end() && linkIdIt->is_number_unsigned() &&
                linkStartIt != linkData.end() && linkStartIt->is_number_unsigned() &&
                linkEndIt != linkData.end() && linkEndIt->is_number_unsigned())
            {
                LinkUniquePtr pLink = std::make_unique<Link>
                (
                    linkIdIt->get<BlueprintId>(),
                    linkStartIt->get<BlueprintId>(),
                    linkEndIt->get<BlueprintId>()
                );
                AddLink(std::move(pLink)); 
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

void Encounter::AddNode(NodeUniquePtr pNode)
{
    for (auto& pin : pNode->Inputs)
    {
        m_PinIdToPinMap[pin.ID.Get()] = &pin;
    }

    for (auto& pin : pNode->Outputs)
    {
        m_PinIdToPinMap[pin.ID.Get()] = &pin;
    }

    m_Nodes.emplace_back(std::move(pNode));
}

bool Encounter::RemoveNode(BlueprintId id)
{
    auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(), [id](const NodeUniquePtr& node) { return node->ID.Get() == id; });
    if (it != m_Nodes.end())
    {
        for (auto& pin : (*it)->Inputs)
        {
            m_PinIdToPinMap.erase(pin.ID.Get());
        }

        for (auto& pin : (*it)->Outputs)
        {
            m_PinIdToPinMap.erase(pin.ID.Get());
        }
        
        m_Nodes.erase(it);
        return true;
    }
    return false;
}

const std::vector<Link*> Encounter::GetLinks() const
{
    std::vector<Link*> links;
    for (const auto& link : m_Links)
    {
        links.push_back(link.get());
    }
    return links;
}

bool Encounter::RemoveLink(BlueprintId id)
{
    auto it = std::find_if(m_Links.begin(), m_Links.end(), [id](const LinkUniquePtr& link) { return link->ID.Get() == id; });
    if (it != m_Links.end())
    {
        m_Links.erase(it);
        return true;
    }
    return false;
}

Pin* Encounter::GetPin(BlueprintId id) const
{
    auto it = m_PinIdToPinMap.find(id);
    if (it != m_PinIdToPinMap.end())
    {
        return it->second;
    }
    return nullptr;
}

} // namespace WingsOfSteel::TheBrightestStar
