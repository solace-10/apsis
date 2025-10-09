#include "sector/deck/deploy_escort_card.hpp"

#include <magic_enum.hpp>

#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <pandora.hpp>

#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "systems/carrier_system.hpp"
#include "game.hpp"

namespace WingsOfSteel
{

namespace
{
    constexpr const char* kNameKey = "name";
    constexpr const char* kEscortsKey = "escorts";
    constexpr const char* kWingSizeKey = "wing_size";
    constexpr const char* kWingRoleKey = "wing_role";
}

bool DeployEscortCard::Deserialize(const ResourceDataStore* pContext, const Json::Data& data)
{
    const auto name = Json::DeserializeString(pContext, data, kNameKey);
    if (!name.has_value())
    {
        return false;
    }
    m_Name = name.value();

    const auto escorts = Json::DeserializeArray(pContext, data, kEscortsKey);
    if (!escorts.has_value())
    {
        return false;
    }
    
    for (auto& escort : escorts.value())
    {
        m_Escorts.push_back(escort.get<std::string>());
    }

    const auto wingRole = Json::DeserializeString(pContext, data, kWingRoleKey);
    if (!wingRole.has_value())
    {
        return false;
    }

    const auto roleOpt = magic_enum::enum_cast<WingRole>(wingRole.value(), magic_enum::case_insensitive);
    if (!roleOpt.has_value())
    {
        return false;
    }

    m_WingRole = roleOpt.value();
    return true;
}

Card::Type DeployEscortCard::GetType() const
{
    return Card::Type::DeployEscort;
}

const std::string& DeployEscortCard::GetName() const
{
    return m_Name;
}

void DeployEscortCard::Play()
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    EntitySharedPtr pCarrier = pSector->GetEncounter()->GetCarrier();
    CarrierSystem* pCarrierSystem = pSector->GetSystem<CarrierSystem>();
    if (!pCarrier || !pCarrierSystem)
    {
        return;
    }

    pCarrierSystem->LaunchEscorts(pCarrier, m_Escorts, m_WingRole);
}

} // namespace WingsOfSteel
