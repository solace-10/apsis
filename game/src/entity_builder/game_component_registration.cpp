#include <scene/components/component_factory_impl.hpp>

// Include all game component headers
#include "components/hardpoint_component.hpp"
#include "components/mech_engine_component.hpp"
#include "components/mech_navigation_component.hpp"
#include "components/name_component.hpp"
#include "components/player_controller_component.hpp"
#include "components/sector_camera_component.hpp"
#include "components/ship_engine_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/weapon_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{
    using namespace Pandora;

    // Register entity adders for all Game components
    void RegisterAllGameComponentsEntityIntegration()
    {
        ComponentFactory::RegisterEntityAdder<HardpointComponent>("hardpoint");
        ComponentFactory::RegisterEntityAdder<MechEngineComponent>("mech_engine");
        ComponentFactory::RegisterEntityAdder<MechNavigationComponent>("mech_navigation");
        ComponentFactory::RegisterEntityAdder<NameComponent>("name");
        ComponentFactory::RegisterEntityAdder<PlayerControllerComponent>("player_controller");
        ComponentFactory::RegisterEntityAdder<SectorCameraComponent>("sector_camera");
        ComponentFactory::RegisterEntityAdder<ShipEngineComponent>("ship_engine");
        ComponentFactory::RegisterEntityAdder<ShipNavigationComponent>("ship_navigation");
        ComponentFactory::RegisterEntityAdder<WeaponComponent>("weapon");
    }

    // Call the registration function during static initialization
    static struct GameEntityIntegrationInitializer {
        GameEntityIntegrationInitializer() {
            RegisterAllGameComponentsEntityIntegration();
        }
    } g_gameEntityIntegrationInitializer;

} // namespace WingsOfSteel::TheBrightestStar