#pragma once

#include <concepts>
#include <random>
#include <stdint.h>

#include "core/assert.hpp"

namespace WingsOfSteel
{

class Random
{
public:
    static void Initialize();

    template <std::integral T>
    static T Get(T max)
    {
        std::uniform_int_distribution<T> distribution(0, max);
        return distribution(m_DefaultRandomEngine);
    }

    template <std::integral T>
    static T Get(T min, T max)
    {
        std::uniform_int_distribution<T> distribution(min, max);
        return distribution(m_DefaultRandomEngine);
    }

    template <std::floating_point T>
    static T Get(T max)
    {
        std::uniform_real_distribution<T> distribution(0, max);
        return distribution(m_DefaultRandomEngine);
    }

    template <std::floating_point T>
    static T Get(T min, T max)
    {
        std::uniform_real_distribution<T> distribution(min, max);
        return distribution(m_DefaultRandomEngine);
    }

    // Bates distribution: the mean of `samples` uniform draws from [min, max]. Values bunch up
    // around the middle of the range in a bell curve, which suits anything that should have a
    // typical value with rarer extremes, such as how large a plant grows.
    //
    // Unlike a normal distribution it is bounded by construction, so it never needs clamping and
    // never piles results up exactly on min or max. `samples` controls how tight the bell is: the
    // standard deviation is (max - min) / sqrt(12 * samples), so 2 barely favours the middle, 3
    // still reaches the extremes occasionally, and 8 keeps nearly everything near the centre.
    template <std::floating_point T>
    static T GetBates(T min, T max, uint32_t samples = 3)
    {
        PANDORA_ASSERT(samples > 0) << "samples=" << samples;

        T total = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            total += Get(min, max);
        }
        return total / static_cast<T>(samples);
    }

private:
    static std::default_random_engine m_DefaultRandomEngine;
};

} // namespace WingsOfSteel