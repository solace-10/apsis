#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

#include "imgui/imgui_system.hpp"

#include "imgui/fonts/dejavu_sans_mono_data.hpp"
#include "imgui/fonts/icons_font_awesome.hpp"
#include "imgui/fonts/icons_font_awesome_data.hpp"
#include "render/debug_render.hpp"
#include "render/debug_render_demo.hpp"
#include "render/rendersystem.hpp"
#include "render/shader_editor.hpp"
#include "render/window.hpp"
#include "resources/resource_font.hpp"
#include "resources/resource_system.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

ImGuiSystem::ImGuiSystem()
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

    AddDefaultFont();
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

            if (ImGui::MenuItem("Shader editor", nullptr, &m_ShowShaderEditor))
            {
                GetRenderSystem()->GetShaderEditor()->Show(m_ShowShaderEditor);
            }

            ImGui::EndMenu();
        }

        if (m_GameMenuBarCallback)
        {
            m_GameMenuBarCallback();
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

void ImGuiSystem::SetGameMenuBarCallback(ImGuiGameMenuBarCallback callback)
{
    m_GameMenuBarCallback = callback;
}

void ImGuiSystem::AddDefaultFont()
{
    const float defaultFontSize = 16.0f;
    const float defaultIconsFontSize = 18.0f;

    ImFontAtlas* pFontAtlas = ImGui::GetIO().Fonts;
    m_pDefaultFont = pFontAtlas->AddFontFromMemoryCompressedTTF(sDejaVuSansMonoCompressedData, sDejaVuSansMonoCompressedSize, defaultFontSize);

    // Merge FontAwesome into the default font. 
    const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode = true;
    iconsConfig.GlyphMinAdvanceX = defaultFontSize;
    iconsConfig.GlyphOffset = ImVec2(0.0f, (defaultIconsFontSize - defaultFontSize) / 2.0f);
    pFontAtlas->AddFontFromMemoryCompressedTTF(sFontAwesomeCompressedData, sFontAwesomeCompressedSize, defaultIconsFontSize, &iconsConfig, iconsRanges);

    pFontAtlas->Build();
}

} // namespace WingsOfSteel::Pandora