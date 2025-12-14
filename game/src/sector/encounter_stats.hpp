#pragma once

#include <chrono>

namespace WingsOfSteel
{

struct EncounterStats
{
    std::chrono::steady_clock::time_point startedAt{};
    std::chrono::steady_clock::time_point finishedAt{};
    int kills{ 0 };
};

} // namespace WingsOfSteel
