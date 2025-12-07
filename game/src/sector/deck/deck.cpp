#include "sector/deck/deck.hpp"

#include <algorithm>
#include <random>

#include <core/log.hpp>
#include <core/serialization.hpp>
#include <pandora.hpp>

#include "sector/deck/card.hpp"
#include "sector/deck/deploy_escort_card.hpp"

namespace WingsOfSteel
{

namespace
{
    constexpr const char* kDeckKey = "deck";
    constexpr const char* kCardKey = "card";
    constexpr const char* kCardTypeKey = "type";
    constexpr const char* kCountKey = "count";
} // namespace

void Deck::Initialize(ResourceDataStore* pResource, const Json::Data& tierObject)
{
    const auto deckArray = Json::DeserializeArray(pResource, tierObject, kDeckKey);
    if (!deckArray.has_value())
    {
        return;
    }

    for (const auto& cardEntryObject : deckArray.value())
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
