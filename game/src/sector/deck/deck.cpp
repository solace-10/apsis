#include "sector/deck/deck.hpp"

#include <algorithm>
#include <sstream>
#include <random>

#include <core/serialization.hpp>
#include <core/log.hpp>
#include <pandora.hpp>

#include "sector/deck/deploy_escort_card.hpp"
#include "sector/deck/card.hpp"

namespace WingsOfSteel
{

namespace
{
    constexpr const char* kDeckKey = "deck";
    constexpr const char* kCardKey = "card";
    constexpr const char* kCardTypeKey = "type";
    constexpr const char* kCountKey = "count";
}

void Deck::Initialize(ResourceDataStore* pResource, uint32_t tier)
{
    const Json::Data& data = pResource->Data();

    const auto deckObject = Json::DeserializeObject(pResource, data, kDeckKey);
    if (!deckObject.has_value())
    {
        return;
    }

    std::stringstream tierKeyStream;
    tierKeyStream << "tier" << tier;
    const std::string tierKey(tierKeyStream.str());
    const auto tierArray = Json::DeserializeArray(pResource, deckObject.value(), tierKey);
    if (!tierArray.has_value())
    {
        return;
    }

    for (const auto& cardEntryObject : tierArray.value())
    {
        const auto cardObject = Json::DeserializeObject(pResource, cardEntryObject, kCardKey);
        if (!cardObject.has_value())
        {
            return;
        }

        const std::string cardInstanceType = Json::DeserializeString(pResource, cardObject.value(), kCardTypeKey);
        if (cardInstanceType.empty())
        {
            return;
        }
        CardSharedPtr pCard;
        if (cardInstanceType == "deploy_escort")
        {
            pCard = std::make_shared<DeployEscortCard>();
            if (!pCard->Deserialize(pResource, cardObject.value()))
            {
                Log::Error() << pResource->GetPath() << ": failed to deserialize card.";
                return;
            }
        }
        else
        {
            Log::Error() << pResource->GetPath() << ": unknown card type '" << cardInstanceType << "'.";
            return;
        }

        if (pCard)
        {
            const uint32_t cardCount = Json::DeserializeUnsignedInteger(pResource, cardEntryObject, kCountKey);
            for (uint32_t count = 0; count < cardCount; count++)
            {
                m_Cards.emplace_back(pCard, false);
            }
        }
    }

    ShuffleAndReset();
}

bool Deck::PlayNextCard()
{
    // Find the first card that hasn't been played yet
    for (auto& [pCard, played] : m_Cards)
    {
        if (!played)
        {
            pCard->Play();
            played = true;
            return true;
        }
    }

    // No cards left to play
    return false;
}

void Deck::ShuffleAndReset()
{
    // Mark all cards as not played
    for (auto& [pCard, played] : m_Cards)
    {
        played = false;
    }

    // Shuffle the cards
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(m_Cards.begin(), m_Cards.end(), gen);
}

std::vector<std::pair<Card*, bool>> Deck::GetAllCards() const
{
    std::vector<std::pair<Card*, bool>> result;
    result.reserve(m_Cards.size());

    for (const auto& [pCard, played] : m_Cards)
    {
        result.emplace_back(pCard.get(), played);
    }

    return result;
}

} // namespace WingsOfSteel
