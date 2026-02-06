#include <chrono>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <pandora.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/orbital_elements_component.hpp"
#include "components/orbital_state_component.hpp"
#include "systems/orbit_simulation_system.hpp"

namespace WingsOfSteel
{

// Earth's gravitational parameter (km³/s²)
static constexpr double kMu = 398600.4418;

// Earth's mean radius (km)
static constexpr double kEarthMeanRadius = 6371.0;

// Earth's angular velocity (rad/s)
static constexpr double kEarthAngularVelocity = 7.2921159e-5;

OrbitSimulationSystem::OrbitSimulationSystem()
{
}

OrbitSimulationSystem::~OrbitSimulationSystem()
{
}

void OrbitSimulationSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const OrbitalElementsComponent, OrbitalStateComponent, TransformComponent>();

    view.each([&registry](const OrbitalElementsComponent& orbitalElements, OrbitalStateComponent& orbitalState, TransformComponent& transformComponent) {
        const glm::dvec3 position = CalculateCartesianPosition(orbitalElements); // Position is in km, in ECI coordinates
        transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(position.y, position.z, position.x));

        // Update orbital state component
        orbitalState.m_PositionECI = position;

        // Calculate semi-major axis from mean motion: n = sqrt(mu/a³) => a = (mu/n²)^(1/3)
        const double n = orbitalElements.GetMeanMotion() * 2.0 * glm::pi<double>() / 86400.0;
        orbitalState.m_SemiMajorAxis = std::cbrt(kMu / (n * n));

        // Calculate altitude
        const double r = glm::length(position);
        orbitalState.m_Altitude = r - kEarthMeanRadius;

        // Calculate velocity from vis-viva equation: v² = μ(2/r - 1/a)
        orbitalState.m_Velocity = std::sqrt(kMu * (2.0 / r - 1.0 / orbitalState.m_SemiMajorAxis));

        // Calculate lat/lon
        const glm::dvec2 latLon = ECIToLatLon(position);
        orbitalState.m_Latitude = latLon.x;
        orbitalState.m_Longitude = latLon.y;
    });
}

// Calculate Cartesian position (in km) from Keplerian orbital elements
// Propagates the position to the current system time
glm::dvec3 OrbitSimulationSystem::CalculateCartesianPosition(const OrbitalElementsComponent& orbitalElements)
{
    // Convert mean motion from rev/day to rad/s
    const double n = orbitalElements.GetMeanMotion() * 2.0 * glm::pi<double>() / 86400.0;

    // Calculate semi-major axis from mean motion: n = sqrt(mu/a³) => a = (mu/n²)^(1/3)
    const double a = std::cbrt(kMu / (n * n));

    const double e = orbitalElements.GetEccentricity();

    // Convert angles from degrees to radians
    const double i = glm::radians(static_cast<double>(orbitalElements.GetInclination()));
    const double omega = glm::radians(static_cast<double>(orbitalElements.GetRightAscensionOfAscendingNode())); // RAAN (Ω)
    const double w = glm::radians(static_cast<double>(orbitalElements.GetArgumentOfPericenter())); // Argument of pericenter (ω)
    const double M_epoch = glm::radians(static_cast<double>(orbitalElements.GetMeanAnomaly()));

    // Propagate mean anomaly to current time
    const auto now = std::chrono::system_clock::now();
    const auto epoch = orbitalElements.GetEpoch();
    const double deltaSeconds = std::chrono::duration<double>(now - epoch).count();
    double M = M_epoch + n * deltaSeconds;

    // Normalize to [0, 2π]
    M = std::fmod(M, 2.0 * glm::pi<double>());
    if (M < 0.0)
    {
        M += 2.0 * glm::pi<double>();
    }

    // Solve Kepler's equation to get Eccentric Anomaly
    const double E = SolveKeplerEquation(M, e);

    // Calculate True Anomaly from Eccentric Anomaly
    // tan(nu/2) = sqrt((1+e)/(1-e)) * tan(E/2)
    const double nu = 2.0 * std::atan2(std::sqrt(1.0 + e) * std::sin(E / 2.0), std::sqrt(1.0 - e) * std::cos(E / 2.0));

    // Calculate orbital radius
    const double r = a * (1.0 - e * std::cos(E));

    // Position in perifocal (orbital plane) coordinates
    const double x_pf = r * std::cos(nu);
    const double y_pf = r * std::sin(nu);

    // Transform from perifocal to ECI (Earth-Centered Inertial) coordinates
    // Using the rotation: R = R_z(Ω) * R_x(i) * R_z(ω)
    const double cos_omega = std::cos(omega);
    const double sin_omega = std::sin(omega);
    const double cos_i = std::cos(i);
    const double sin_i = std::sin(i);
    const double cos_w = std::cos(w);
    const double sin_w = std::sin(w);

    // Combined rotation matrix elements (optimized form)
    const double x = x_pf * (cos_omega * cos_w - sin_omega * sin_w * cos_i)
        + y_pf * (-cos_omega * sin_w - sin_omega * cos_w * cos_i);

    const double y = x_pf * (sin_omega * cos_w + cos_omega * sin_w * cos_i)
        + y_pf * (-sin_omega * sin_w + cos_omega * cos_w * cos_i);

    const double z = x_pf * (sin_w * sin_i)
        + y_pf * (cos_w * sin_i);

    return glm::dvec3(x, y, z);
}

// Calculate Greenwich Mean Sidereal Time (GMST) in radians.
// Based on the current UTC time.
double OrbitSimulationSystem::CalculateGMST()
{
    const auto now = std::chrono::system_clock::now();
    const auto duration = now.time_since_epoch();

    // Convert to days since J2000.0 epoch (January 1, 2000, 12:00 TT)
    // J2000.0 in Unix time is 946728000 seconds (2000-01-01 12:00:00 UTC approximately)
    constexpr double kJ2000UnixSeconds = 946728000.0;
    const double secondsSinceJ2000 = std::chrono::duration<double>(duration).count() - kJ2000UnixSeconds;
    const double daysSinceJ2000 = secondsSinceJ2000 / 86400.0;

    // GMST at J2000.0 is approximately 280.46061837 degrees
    // Earth rotates approximately 360.98564736629 degrees per day
    double gmstDegrees = 280.46061837 + 360.98564736629 * daysSinceJ2000;

    // Normalize to [0, 360)
    gmstDegrees = std::fmod(gmstDegrees, 360.0);
    if (gmstDegrees < 0.0)
    {
        gmstDegrees += 360.0;
    }

    return glm::radians(gmstDegrees);
}

// Convert ECI coordinates to latitude and longitude (in degrees)
// Returns: x = latitude, y = longitude
glm::dvec2 OrbitSimulationSystem::ECIToLatLon(const glm::dvec3& eciPosition)
{
    const double gmst = CalculateGMST();

    // Convert ECI to ECEF by rotating around Z-axis by -GMST
    const double cosGmst = std::cos(gmst);
    const double sinGmst = std::sin(gmst);

    const double xEcef = eciPosition.x * cosGmst + eciPosition.y * sinGmst;
    const double yEcef = -eciPosition.x * sinGmst + eciPosition.y * cosGmst;
    const double zEcef = eciPosition.z;

    // Calculate longitude (in radians, then convert to degrees)
    const double longitude = std::atan2(yEcef, xEcef);

    // Calculate geocentric latitude (in radians, then convert to degrees)
    const double r_xy = std::sqrt(xEcef * xEcef + yEcef * yEcef);
    const double latitude = std::atan2(zEcef, r_xy);

    return glm::dvec2(glm::degrees(latitude), glm::degrees(longitude));
}

// Solve Kepler's equation: M = E - e*sin(E)
// Returns Eccentric Anomaly E given Mean Anomaly M and eccentricity e
// Uses Newton-Raphson iteration
double OrbitSimulationSystem::SolveKeplerEquation(double meanAnomaly, double eccentricity, int maxIterations, double tolerance)
{
    double E = meanAnomaly; // Initial guess
    for (int i = 0; i < maxIterations; ++i)
    {
        const double f = E - eccentricity * std::sin(E) - meanAnomaly;
        const double fPrime = 1.0 - eccentricity * std::cos(E);
        const double delta = f / fPrime;
        E -= delta;
        if (std::abs(delta) < tolerance)
        {
            break;
        }
    }
    return E;
}

} // namespace WingsOfSteel
