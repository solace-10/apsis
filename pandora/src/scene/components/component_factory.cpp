#include "component_factory_impl.hpp"

// Include all component headers to trigger entity registrations
#include "camera_component.hpp"
#include "debug_render_component.hpp"
#include "model_component.hpp"
#include "orbit_camera_component.hpp"
#include "rigid_body_component.hpp"
#include "transform_component.hpp"

namespace WingsOfSteel::Pandora
{
    std::unordered_map<std::string, ComponentFactory::CreateFunc>* ComponentFactory::s_creators = nullptr;
    std::unordered_map<std::string, ComponentFactory::EntityAdderFunc>* ComponentFactory::s_entityAdders = nullptr;

    // Register entity adders for all Pandora components
    void RegisterAllPandoraComponentsEntityIntegration()
    {
        ComponentFactory::RegisterEntityAdder<CameraComponent>("camera");
        ComponentFactory::RegisterEntityAdder<DebugRenderComponent>("debug_render");
        ComponentFactory::RegisterEntityAdder<ModelComponent>("model");
        ComponentFactory::RegisterEntityAdder<OrbitCameraComponent>("orbit_camera");
        ComponentFactory::RegisterEntityAdder<RigidBodyComponent>("rigid_body");
        ComponentFactory::RegisterEntityAdder<TransformComponent>("transform");
    }

    // Call the registration function during static initialization
    static struct EntityIntegrationInitializer {
        EntityIntegrationInitializer() {
            RegisterAllPandoraComponentsEntityIntegration();
        }
    } g_entityIntegrationInitializer;

} // namespace WingsOfSteel::Pandora