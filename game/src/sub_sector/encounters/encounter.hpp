#pragma once

#include <vector>

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

private:
    Pandora::ResourceDataStoreSharedPtr m_pDataStore;
    std::vector<Node*> m_Nodes;
};

} // namespace WingsOfSteel::TheBrightestStar