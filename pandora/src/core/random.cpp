#include <chrono>
#include <random>

#include "core/random.hpp"

namespace WingsOfSteel
{

std::default_random_engine Random::m_DefaultRandomEngine;

void Random::Initialize()
{
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    m_DefaultRandomEngine = std::default_random_engine(seed);
}

} // namespace WingsOfSteel