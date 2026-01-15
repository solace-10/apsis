#include <chrono>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/debug_render.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <pandora.hpp>

#include "systems/orbit_simulation_system.hpp"
#include "components/space_object_component.hpp"

namespace WingsOfSteel
{

namespace
{

// Earth's gravitational parameter (km³/s²)
constexpr double kMu = 398600.4418;

// Solve Kepler's equation: M = E - e*sin(E)
// Returns Eccentric Anomaly E given Mean Anomaly M and eccentricity e
// Uses Newton-Raphson iteration
double SolveKeplerEquation(double meanAnomaly, double eccentricity, int maxIterations = 10, double tolerance = 1e-10)
{
    double E = meanAnomaly; // Initial guess
    for (int i = 0; i < maxIterations; ++i)
    {
        double f = E - eccentricity * std::sin(E) - meanAnomaly;
        double fPrime = 1.0 - eccentricity * std::cos(E);
        double delta = f / fPrime;
        E -= delta;
        if (std::abs(delta) < tolerance)
        {
            break;
        }
    }
    return E;
}

// Calculate Cartesian position (in km) from Keplerian orbital elements
// Propagates the position to the current system time
glm::dvec3 CalculateCartesianPosition(const SpaceObject& spaceObject)
{
    // Convert mean motion from rev/day to rad/s
    double n = spaceObject.GetMeanMotion() * 2.0 * glm::pi<double>() / 86400.0;

    // Calculate semi-major axis from mean motion: n = sqrt(mu/a³) => a = (mu/n²)^(1/3)
    double a = std::cbrt(kMu / (n * n));

    double e = spaceObject.GetEccentricity();

    // Convert angles from degrees to radians
    double i = glm::radians(static_cast<double>(spaceObject.GetInclination()));
    double omega = glm::radians(static_cast<double>(spaceObject.GetRightAscensionOfAscendingNode())); // RAAN (Ω)
    double w = glm::radians(static_cast<double>(spaceObject.GetArgumentOfPericenter())); // Argument of pericenter (ω)
    double M_epoch = glm::radians(static_cast<double>(spaceObject.GetMeanAnomaly()));

    // Propagate mean anomaly to current time
    auto now = std::chrono::system_clock::now();
    auto epoch = spaceObject.GetEpoch();
    double deltaSeconds = std::chrono::duration<double>(now - epoch).count();
    double M = M_epoch + n * deltaSeconds;

    // Normalize to [0, 2π]
    M = std::fmod(M, 2.0 * glm::pi<double>());
    if (M < 0.0) M += 2.0 * glm::pi<double>();

    // Solve Kepler's equation to get Eccentric Anomaly
    double E = SolveKeplerEquation(M, e);

    // Calculate True Anomaly from Eccentric Anomaly
    // tan(nu/2) = sqrt((1+e)/(1-e)) * tan(E/2)
    double nu = 2.0 * std::atan2(
        std::sqrt(1.0 + e) * std::sin(E / 2.0),
        std::sqrt(1.0 - e) * std::cos(E / 2.0)
    );

    // Calculate orbital radius
    double r = a * (1.0 - e * std::cos(E));

    // Position in perifocal (orbital plane) coordinates
    double x_pf = r * std::cos(nu);
    double y_pf = r * std::sin(nu);

    // Transform from perifocal to ECI (Earth-Centered Inertial) coordinates
    // Using the rotation: R = R_z(-Ω) * R_x(-i) * R_z(-ω)
    double cos_omega = std::cos(omega);
    double sin_omega = std::sin(omega);
    double cos_i = std::cos(i);
    double sin_i = std::sin(i);
    double cos_w = std::cos(w);
    double sin_w = std::sin(w);

    // Combined rotation matrix elements (optimized form)
    double x = x_pf * (cos_omega * cos_w - sin_omega * sin_w * cos_i)
             - y_pf * (cos_omega * sin_w + sin_omega * cos_w * cos_i);

    double y = x_pf * (sin_omega * cos_w + cos_omega * sin_w * cos_i)
             - y_pf * (sin_omega * sin_w - cos_omega * cos_w * cos_i);

    double z = x_pf * (sin_w * sin_i)
             + y_pf * (cos_w * sin_i);

    return glm::dvec3(x, y, z);
}

} // anonymous namespace

OrbitSimulationSystem::OrbitSimulationSystem()
{
    
}

OrbitSimulationSystem::~OrbitSimulationSystem()
{
    
}

void OrbitSimulationSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const SpaceObjectComponent, TransformComponent>();

    view.each([&registry](const auto entity, const SpaceObjectComponent& spaceObjectComponent, TransformComponent& transformComponent)
    {
        glm::dvec3 position = CalculateCartesianPosition(spaceObjectComponent.GetSpaceObject()); // Position is in km, in ECI coordinates
        transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, position.z));
    });
}

} // namespace WingsOfSteel
