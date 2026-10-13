#pragma once

#include <array>
#include <cstddef>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ProfilingOverlay);

// A small always-on-top overlay in the top right corner of the screen, showing
// the current frame rate, frame time and a plot of the recent frame times.
class ProfilingOverlay
{
public:
    // Samples the current frame time and, if the overlay is visible, draws it.
    void Update();

    void Show(bool state);
    bool IsVisible() const;

private:
    void Draw();

    static constexpr size_t sSampleCount = 180; // About 3 seconds worth of frames at 60fps.

    std::array<float, sSampleCount> m_FrameTimes{};
    size_t m_Offset{ 0 };
    size_t m_ValidSamples{ 0 };
    bool m_Visible{ false };
};

inline void ProfilingOverlay::Show(bool state)
{
    m_Visible = state;
}

inline bool ProfilingOverlay::IsVisible() const
{
    return m_Visible;
}

} // namespace WingsOfSteel
