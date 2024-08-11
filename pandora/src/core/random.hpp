#pragma once

#include <concepts>
#include <random>
#include <stdint.h>

namespace WingsOfSteel::Pandora
{

class Random
{
public:
    static void Initialize();

    template<std::integral T>
    static T Get(T max)
    {
        std::uniform_int_distribution<T> distribution(0, max);
        return distribution(m_DefaultRandomEngine);
    }

    template<std::integral T>
    static T Get(T min, T max)
    {
        std::uniform_int_distribution<T> distribution(min, max);
        return distribution(m_DefaultRandomEngine);
    }

    template<std::floating_point T>
    static T Get(T max)
    {
        std::uniform_real_distribution<T> distribution(0, max);
        return distribution(m_DefaultRandomEngine);
    }

    template<std::floating_point T>
    static T Get(T min, T max)
    {
        std::uniform_real_distribution<T> distribution(min, max);
        return distribution(m_DefaultRandomEngine);
    }

private:
    static std::default_random_engine m_DefaultRandomEngine;

};

} // namespace WingsOfSteel::Pandora