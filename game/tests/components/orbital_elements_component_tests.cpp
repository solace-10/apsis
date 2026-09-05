#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <core/serialization.hpp>

#include "components/orbital_elements_component.hpp"

using namespace WingsOfSteel;
using Catch::Matchers::WithinAbs;

namespace
{

// The shape the Scaleway function serves, with the column names it serialises. Real values,
// from the ISS element set in game/bin/data/core/celestrak/stations.json.
constexpr const char* kIssJson = R"({
    "norad_id": 25544,
    "epoch": "2026-01-16T08:52:15.244032",
    "mean_motion": 15.49326051,
    "eccentricity": 0.00077838,
    "inclination": 51.6334,
    "raan": 331.0355,
    "arg_of_pericenter": 24.7534,
    "mean_anomaly": 335.3826,
    "bstar": 0.00018173329
})";

OrbitalElementsComponent Deserialize(const char* pJson)
{
    OrbitalElementsComponent component;
    component.Deserialize(nullptr, Json::Data::parse(pJson));
    return component;
}

} // namespace

// BSTAR is the one element of a TLE that SGP4 cannot do without and Keplerian propagation has
// no use for, which is why it was absent until now: it is consumed when an element set is
// initialised and again at every step.
TEST_CASE("An element set carries the drag term SGP4 needs", "[components][orbital_elements]")
{
    const OrbitalElementsComponent component = Deserialize(kIssJson);

    REQUIRE(component.GetNoradId() == 25544);
    REQUIRE_THAT(static_cast<double>(component.GetBStar()), WithinAbs(0.00018173329, 1e-11));
}
