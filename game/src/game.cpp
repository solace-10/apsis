#include <debug_visualization/model_visualization.hpp>
#include <imgui/imgui_system.hpp>
#include <input/input_system.hpp>
#include <pandora.hpp>
#include <render/render_pass/ui_render_pass.hpp>
#include <render/rendersystem.hpp>
#include <scene/camera.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>
#include <scene/systems/model_render_system.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "game.hpp"
#include "render/sector_render_pass.hpp"
#include "sector/sector.hpp"
#include "systems/planet_render_system.hpp"

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
    pRenderSystem->AddRenderPass(std::make_shared<SectorRenderPass>());
    pRenderSystem->AddRenderPass(std::make_shared<UIRenderPass>());

    GetImGuiSystem()->SetGameMenuBarCallback([this]() { DrawImGuiMenuBar(); });

#if defined(TARGET_PLATFORM_WEB)
    GetInputSystem()->SetCursorMode(CursorMode::Locked);
#elif defined(TARGET_PLATFORM_NATIVE)
    GetInputSystem()->SetCursorMode(CursorMode::Normal);
#endif

    m_pSector = std::make_shared<Sector>();
    m_pSector->Initialize();

    SetActiveScene(m_pSector);
}

void Game::Update(float delta)
{
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

        if (ImGui::BeginMenu("Earth"))
        {
            PlanetRenderSystem* pPlanetRenderSystem = m_pSector->GetSystem<PlanetRenderSystem>();
            if (pPlanetRenderSystem)
            {
                bool wireframe = pPlanetRenderSystem->IsWireframeEnabled();
                if (ImGui::MenuItem("Wireframe", nullptr, &wireframe))
                {
                    pPlanetRenderSystem->SetWireframeEnabled(wireframe);
                }
            }
            ImGui::EndMenu();
        }
    }
}

} // namespace WingsOfSteel
