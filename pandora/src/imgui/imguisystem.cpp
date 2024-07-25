#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

#include "imgui/imguisystem.hpp"

#include "render/debug_render.hpp"
#include "render/debug_render_demo.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

ImGuiSystem::ImGuiSystem()
: m_ShowDemoWindow(false)
, m_DebugDrawDemo(false)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(GetWindow()->GetRawWindow(), true);
#if defined(TARGET_PLATFORM_WEB)
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
    ImGui::GetIO().IniFilename = nullptr; // Disable ini files when targetting web, as we don't support writing temporary files.
#endif
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = GetRenderSystem()->GetDevice().Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(GetWindow()->GetTextureFormat());
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);
}

ImGuiSystem::~ImGuiSystem()
{
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiSystem::OnFrameStart()
{
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiSystem::Update()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Engine"))
        {
            if (ImGui::BeginMenu("Demos"))
            {
                ImGui::MenuItem("ImGui demo window", nullptr, &m_ShowDemoWindow);

                if (ImGui::MenuItem("Debug draw demo", nullptr, &m_DebugDrawDemo))
                {
                    GetDebugRender()->GetDemo()->Show(m_DebugDrawDemo);
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (m_ShowDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_ShowDemoWindow);
    }
}
    
void ImGuiSystem::Render(wgpu::RenderPassEncoder& pass)
{
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
}

} // namespace WingsOfSteel::Pandora