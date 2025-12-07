#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource_data_store.hpp>
#include <scene/scene.hpp>

#include "sector/deck/card.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Deck);
class Deck
{
public:
    Deck() = default;
    ~Deck() = default;

    // `pResource`: an encounter from `encounters/difficulty<n>`.
    // `tierObject`: the JSON object for this tier from the "tiers" array.
    void Initialize(ResourceDataStore* pResource, const Json::Data& tierObject);

    // Plays a card from the deck if possible; returns if it was successful.
    bool PlayNextCard();

    // Shuffle all the cards in this deck and mark them as not having been played.
    void ShuffleAndReset();

    // For debug purposes only. Returns a vector of pairs, with the second element in the pair
    // indicating whether the card has been played or not.
    std::vector<std::pair<Card*, bool>> GetAllCards() const;

private:
    std::vector<std::pair<CardSharedPtr, bool>> m_Cards;
};

} // namespace WingsOfSteel