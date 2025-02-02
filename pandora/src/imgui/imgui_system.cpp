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
    ApplyStyle();
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

void ImGuiSystem::ApplyStyle()
{
    ImGuiStyle* pStyle = &ImGui::GetStyle();

    pStyle->WindowBorderSize         = 0.0f;
    pStyle->WindowPadding            = ImVec2(15, 15);
    pStyle->WindowRounding           = 10.0f;
    pStyle->FramePadding             = ImVec2(5, 5);
    pStyle->FrameRounding            = 4.0f;
    pStyle->ItemSpacing              = ImVec2(12, 8);
    pStyle->ItemInnerSpacing         = ImVec2(8, 6);
    pStyle->IndentSpacing            = 25.0f;
    pStyle->ScrollbarSize            = 15.0f;
    pStyle->ScrollbarRounding        = 9.0f;
    pStyle->GrabMinSize              = 5.0f;
    pStyle->GrabRounding             = 3.0f;


    pStyle->Colors[ImGuiCol_Text]                  = ImColor(240, 240, 240, 255);
    pStyle->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.40f, 0.39f, 0.38f, 0.77f);
    pStyle->Colors[ImGuiCol_WindowBg]              = ImColor(46, 46, 46, 240);
    pStyle->Colors[ImGuiCol_ChildBg]               = ImColor(0, 0, 0, 14);
    pStyle->Colors[ImGuiCol_PopupBg]               = ImColor(70, 70, 70, 230);
    pStyle->Colors[ImGuiCol_Border]                = ImColor(5, 250, 191, 50);
    pStyle->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
    pStyle->Colors[ImGuiCol_FrameBg]               = ImColor(70, 70, 70, 255);
    pStyle->Colors[ImGuiCol_FrameBgHovered]        = ImColor(80, 80, 80, 255);
    pStyle->Colors[ImGuiCol_FrameBgActive]         = ImColor(90, 90, 90, 255);
    pStyle->Colors[ImGuiCol_TitleBg]               = ImColor(70, 70, 70, 255);
    pStyle->Colors[ImGuiCol_TitleBgCollapsed]      = ImColor(80, 80, 80, 255);
    pStyle->Colors[ImGuiCol_TitleBgActive]         = ImColor(90, 90, 90, 255);
    pStyle->Colors[ImGuiCol_MenuBarBg]             = ImColor(70, 70, 70, 255);
    pStyle->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(1.00f, 0.98f, 0.95f, 0.00f);
    pStyle->Colors[ImGuiCol_ScrollbarGrab]         = ImColor(70, 70, 70, 255);
    pStyle->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImColor(80, 80, 80, 255);
    pStyle->Colors[ImGuiCol_ScrollbarGrabActive]   = ImColor(5, 250, 191, 200);
    //pStyle->Colors[ImGuiCol_ComboBg]               = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
    pStyle->Colors[ImGuiCol_CheckMark]             = ImVec4(0.02f, 0.98f, 0.75f, 0.80f);
    pStyle->Colors[ImGuiCol_SliderGrab]            = ImColor(5, 250, 191, 200);
    pStyle->Colors[ImGuiCol_SliderGrabActive]      = ImColor(5, 250, 191, 255);
    pStyle->Colors[ImGuiCol_Button]                = ImVec4(0.02f, 0.98f, 0.75f, 0.75f);
    pStyle->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.02f, 0.98f, 0.75f, 1.00f);
    pStyle->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.02f, 0.98f, 0.75f, 1.00f);
    pStyle->Colors[ImGuiCol_Header]                = ImColor(70, 70, 70, 255);
    pStyle->Colors[ImGuiCol_HeaderHovered]         = ImColor(5, 250, 191, 200);
    pStyle->Colors[ImGuiCol_HeaderActive]          = ImColor(5, 250, 191, 255);
    //pStyle->Colors[ImGuiCol_Column]                = ImVec4(0.00f, 0.00f, 0.00f, 0.32f);
    //pStyle->Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.02f, 0.98f, 0.75f, 0.78f);
    //pStyle->Colors[ImGuiCol_ColumnActive]          = ImVec4(0.02f, 0.98f, 0.75f, 1.00f);
    pStyle->Colors[ImGuiCol_ResizeGrip]            = ImColor(70, 70, 70, 255);
    pStyle->Colors[ImGuiCol_ResizeGripHovered]     = ImColor(5, 250, 191, 200);
    pStyle->Colors[ImGuiCol_ResizeGripActive]      = ImColor(5, 250, 191, 255);
    //pStyle->Colors[ImGuiCol_CloseButton]           = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
    //pStyle->Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
    //pStyle->Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
    pStyle->Colors[ImGuiCol_PlotLines]             = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    pStyle->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.02f, 0.98f, 0.75f, 1.00f);
    pStyle->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    pStyle->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.02f, 0.98f, 0.75f, 1.00f);
    pStyle->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.02f, 0.98f, 0.75f, 0.43f);
    //pStyle->Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
    pStyle->Colors[ImGuiCol_Separator]             = ImColor(5, 250, 191, 255);
}

} // namespace WingsOfSteel::Pandora