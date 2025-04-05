#pragma once

#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "encounter_blueprint_types.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(EncounterWindow);

DECLARE_SMART_PTR(Encounter);
class Encounter
{
public:
    Encounter(const std::string& name, Pandora::ResourceDataStoreSharedPtr pDataStore);
    ~Encounter();

    void Start();
    bool IsPlaying() const;
    void Stop();
    void Update(float delta);

    void Save();
    void Revert();

    const std::string& GetName() const;

    void AddNode(NodeUniquePtr pNode);
    bool RemoveNode(BlueprintId id);
    const std::vector<Node*> GetNodes() const;

    void AddLink(LinkUniquePtr pLink);
    bool RemoveLink(BlueprintId id);
    const std::vector<Link*> GetLinks() const;
    const std::vector<Node*> GetLinkedNodes(Pin* pPin) const;
    Node* GetFirstLinkedNode(Pin* pPin) const;

    Pin* GetPin(BlueprintId id) const;

    void Bind(EncounterWindowSharedPtr pEncounterWindow);
    EncounterWindow* GetEncounterWindow() const;

protected:
    Node* GetNode(BlueprintId id) const;

private:
    Node* GetLinkedNode(Pin* pPin, Link* pLink) const;
    void Load();

    Pandora::ResourceDataStoreSharedPtr m_pDataStore;
    std::vector<NodeUniquePtr> m_Nodes;
    std::vector<LinkUniquePtr> m_Links;
    std::unordered_map<BlueprintId, Pin*> m_PinIdToPinMap;
    Node* m_pCurrentNode{nullptr};
    bool m_Started{false};
    std::string m_Name;
    EncounterWindowWeakPtr m_pEncounterWindow;
};

inline bool Encounter::IsPlaying() const
{
    return m_Started;
}

inline void Encounter::Stop()
{
    m_Started = false;
    m_pCurrentNode = nullptr;
}

inline const std::string& Encounter::GetName() const
{
    return m_Name;
}

inline void Encounter::Bind(EncounterWindowSharedPtr pEncounterWindow)
{
    m_pEncounterWindow = pEncounterWindow;
}

inline EncounterWindow* Encounter::GetEncounterWindow() const
{
    return m_pEncounterWindow.lock().get();
}

} // namespace WingsOfSteel::TheBrightestStar