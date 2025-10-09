#pragma once

#include <memory>

#include "sector/deck/card.hpp"

#include "components/wing_component.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(DeployEscortCard);
class DeployEscortCard : public Card, public std::enable_shared_from_this<DeployEscortCard>
{
public:
    DeployEscortCard() = default;
    ~DeployEscortCard() = default;

    bool Deserialize(const ResourceDataStore* pContext, const Json::Data& data) override;
    Card::Type GetType() const override;
    const std::string& GetName() const override;
    void Play() override;

private:
    std::string m_Name;
    std::vector<std::string> m_Escorts;
    WingRole m_WingRole{ WingRole::None };
};

} // namespace WingsOfSteel