#include <algorithm>
#include <cfloat>

#include "imgui/profiling_overlay.hpp"

#include "imgui/imgui.hpp"
#include "imgui/imgui_system.hpp"
#include "pandora.hpp"

namespace WingsOfSteel
{

void ProfilingOverlay::Update()
{
    // Keep sampling even while hidden, so the plot is already populated when the
    // overlay is turned on.
    m_FrameTimes[m_Offset] = ImGui::GetIO().DeltaTime * 1000.0f;
    m_Offset = (m_Offset + 1) % sSampleCount;
    m_ValidSamples = std::min(m_ValidSamples + 1, sSampleCount);

    if (m_Visible)
    {
        Draw();
    }
}

void ProfilingOverlay::Draw()
{
    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    const float padding = 8.0f;

    // WorkPos already excludes the main menu bar, so this sits just below it.
    const ImVec2 position(pViewport->WorkPos.x + pViewport->WorkSize.x - padding, pViewport->WorkPos.y + padding);
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.6f);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Profiling", nullptr, flags))
    {
        float minFrameTime = FLT_MAX;
        float maxFrameTime = 0.0f;
        float totalFrameTime = 0.0f;
        for (size_t i = 0; i < m_ValidSamples; i++)
        {
            const float frameTime = m_FrameTimes[i];
            minFrameTime = std::min(minFrameTime, frameTime);
            maxFrameTime = std::max(maxFrameTime, frameTime);
            totalFrameTime += frameTime;
        }

        const float averageFrameTime = totalFrameTime / static_cast<float>(m_ValidSamples);
        const float framerate = ImGui::GetIO().Framerate;

        ImGui::PushFont(GetImGuiSystem()->GetFont(Font::UBUNTU_MONO_18));
        ImGui::Text("%6.1f FPS  %7.2f ms", framerate, framerate > 0.0f ? 1000.0f / framerate : 0.0f);
        ImGui::Text("min %5.2f  avg %5.2f  max %5.2f", minFrameTime, averageFrameTime, maxFrameTime);

        // Keep the vertical scale anchored to two frames at 60fps, so a steady frame
        // rate doesn't cause the plot to wildly auto-scale on minor variations.
        const float scaleMax = std::max(33.4f, maxFrameTime);
        const bool bufferFull = (m_ValidSamples == sSampleCount);
        ImGui::PlotLines("##frametime", m_FrameTimes.data(), static_cast<int>(m_ValidSamples), bufferFull ? static_cast<int>(m_Offset) : 0, nullptr, 0.0f, scaleMax, ImVec2(220.0f, 48.0f));
        ImGui::PopFont();
    }
    ImGui::End();
}

} // namespace WingsOfSteel
