#include <debug_visualization/model_visualization.hpp>
#include <imgui/imgui_system.hpp>
#include <input/input_system.hpp>
#include <pandora.hpp>
#include <render/render_pass/base_render_pass.hpp>
#include <render/render_pass/ui_render_pass.hpp>
#include <render/rendersystem.hpp>
#include <scene/camera.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>
#include <scene/systems/model_render_system.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "game.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "ui/prefab_editor.hpp"
#include "systems/ai_strategic_system.hpp"
#include "render_pass/threat_indicator_render_pass.hpp"

namespace WingsOfSteel
{

Game* g_pGame = nullptr;

Game::Game()
{
}

Game::~Game()
{
}

Game* Game::Get()
{
    return g_pGame;
}

void Game::Initialize()
{
    g_pGame = this;

    RenderSystem* pRenderSystem = GetRenderSystem();
    pRenderSystem->ClearRenderPasses();
    pRenderSystem->AddRenderPass(std::make_shared<BaseRenderPass>());
    pRenderSystem->AddRenderPass(std::make_shared<ThreatIndicatorRenderPass>());
    pRenderSystem->AddRenderPass(std::make_shared<UIRenderPass>());

    GetImGuiSystem()->SetGameMenuBarCallback([this]() { DrawImGuiMenuBar(); });

#if defined(TARGET_PLATFORM_WEB)
    GetInputSystem()->SetCursorMode(CursorMode::Locked);
#elif defined(TARGET_PLATFORM_NATIVE)
    GetInputSystem()->SetCursorMode(CursorMode::Normal);
    m_pPrefabEditor = std::make_unique<UI::PrefabEditor>();
    m_pPrefabEditor->Initialize();
#endif

    m_pSector = std::make_shared<Sector>();
    m_pSector->Initialize();

    SetActiveScene(m_pSector);
}

void Game::Update(float delta)
{
#if defined(TARGET_PLATFORM_NATIVE)
    m_pPrefabEditor->DrawPrefabEditor();
#endif
}

void Game::Shutdown()
{
}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Game::DrawImGuiMenuBar()
{
    using namespace WingsOfSteel;

    if (m_pSector)
    {
        if (ImGui::BeginMenu("Sector"))
        {
            if (ImGui::BeginMenu("AI"))
            {
                AIStrategicSystem* pStrategicSystem = m_pSector->GetSystem<AIStrategicSystem>();
                if (pStrategicSystem)
                {
                    bool showDebugUI = pStrategicSystem->IsDebugUIVisible();
                    if (ImGui::MenuItem("Strategic layer", nullptr, &showDebugUI))
                    {
                        pStrategicSystem->ShowDebugUI(showDebugUI);
                    }
                }

                Encounter* pEncounter = m_pSector->GetEncounter();
                if (pEncounter)
                {
                    bool showDebugUI = pEncounter->IsDebugUIVisible();
                    if (ImGui::MenuItem("Encounter", nullptr, &showDebugUI))
                    {
                        pEncounter->ShowDebugUI(showDebugUI);
                    }
                }

                ImGui::EndMenu();
            }

            static bool sShowCameraWindow = false;
            if (ImGui::MenuItem("Camera", nullptr, &sShowCameraWindow))
            {
                m_pSector->ShowCameraDebugUI(sShowCameraWindow);
            }
            if (ImGui::BeginMenu("Models"))
            {
                ModelRenderSystem* pModelRenderSystem = m_pSector->GetSystem<ModelRenderSystem>();
                if (pModelRenderSystem)
                {
                    ModelVisualization* pVisualization = pModelRenderSystem->GetVisualization();
                    if (pVisualization)
                    {
                        ImGui::SeparatorText("Debug rendering");
                        bool attachments = pVisualization->IsEnabled(ModelVisualization::Mode::AttachmentPoints);
                        if (ImGui::MenuItem("Attachment points", nullptr, &attachments))
                        {
                            pVisualization->SetEnabled(ModelVisualization::Mode::AttachmentPoints, attachments);
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Physics"))
            {
                PhysicsSimulationSystem* pPhysicsSystem = m_pSector->GetSystem<PhysicsSimulationSystem>();
                if (pPhysicsSystem)
                {
                    PhysicsVisualization* pVisualization = pPhysicsSystem->GetVisualization();
                    if (pVisualization)
                    {
                        ImGui::SeparatorText("Debug rendering");
                        bool wireframe = pVisualization->IsEnabled(PhysicsVisualization::Mode::Wireframe);
                        if (ImGui::MenuItem("Wireframe", nullptr, &wireframe))
                        {
                            pVisualization->SetEnabled(PhysicsVisualization::Mode::Wireframe, wireframe);
                        }

                        bool aabb = pVisualization->IsEnabled(PhysicsVisualization::Mode::AABB);
                        if (ImGui::MenuItem("AABB", nullptr, &aabb))
                        {
                            pVisualization->SetEnabled(PhysicsVisualization::Mode::AABB, aabb);
                        }

                        bool transforms = pVisualization->IsEnabled(PhysicsVisualization::Mode::Transforms);
                        if (ImGui::MenuItem("Transforms", nullptr, &transforms))
                        {
                            pVisualization->SetEnabled(PhysicsVisualization::Mode::Transforms, transforms);
                        }

                        bool rayTests = pVisualization->IsEnabled(PhysicsVisualization::Mode::RayTests);
                        if (ImGui::MenuItem("Ray tests", nullptr, &rayTests))
                        {
                            pVisualization->SetEnabled(PhysicsVisualization::Mode::RayTests, rayTests);
                        }

                        bool contactPoints = pVisualization->IsEnabled(PhysicsVisualization::Mode::ContactPoints);
                        if (ImGui::MenuItem("Contact points", nullptr, &contactPoints))
                        {
                            pVisualization->SetEnabled(PhysicsVisualization::Mode::ContactPoints, contactPoints);
                        }
                    }
                }
                ImGui::EndMenu();
            }
            static bool sShowGrid = true;
            if (ImGui::MenuItem("Grid", nullptr, &sShowGrid))
            {
                m_pSector->ShowGrid(sShowGrid);
            }
            ImGui::EndMenu();
        }
    }

    if (ImGui::BeginMenu("UI"))
    {
#if defined(TARGET_PLATFORM_NATIVE)
        static bool sShowPrefabEditor = false;
        if (ImGui::MenuItem("Prefab Editor", nullptr, &sShowPrefabEditor))
        {
            m_pPrefabEditor->ShowPrefabEditor(sShowPrefabEditor);
        }
#endif
        ImGui::EndMenu();
    }
}

} // namespace WingsOfSteel
