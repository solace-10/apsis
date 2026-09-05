#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <SGP4.h>

#include "components/orbital_elements_component.hpp"

namespace WingsOfSteel::Test
{

// One element set from SGP4-VER.TLE, already initialised.
//
// The three numbers on the end of line 2 are not part of a TLE: they are the propagation
// range the verification file asks for, and only the 'v' typerun of twoline2rv reads them.
struct VerificationCase
{
    std::string satnum;
    double startMinutes{ 0.0 };
    double stopMinutes{ 0.0 };
    double stepMinutes{ 0.0 };
    elsetrec satrec{};
};

// One row of tcppver.out. Later rows carry classical elements and a calendar date as well,
// which say nothing the position and velocity do not.
struct VerificationStep
{
    double tsince{ 0.0 };
    double position[3]{};
    double velocity[3]{};
};

// One "<satnum> xx" block and the rows beneath it.
struct VerificationBlock
{
    std::string satnum;
    std::vector<VerificationStep> steps;
};

// Both are returned in file order rather than keyed by satellite number: 20413 appears
// twice, propagated over two different ranges, so a map would silently drop a case.
std::vector<VerificationCase> LoadVerificationCases(const std::filesystem::path& path);
std::vector<VerificationBlock> LoadVerificationOutput(const std::filesystem::path& path);

// Restates an initialised element set as the JSON the game receives from the server, so that
// our own code can be measured against the reference on identical inputs. Going through
// Deserialize() rather than adding setters keeps the component's only entry point the one
// production uses - and its context pointer is null-safe throughout, so nothing else has to
// be brought up to do it.
//
// The conversion back out of SGP4's units is Vallado's own, inverted: it is what makes this a
// check of MakeSGP4Elements() rather than a restatement of it.
OrbitalElementsComponent AsComponent(const elsetrec& satrec);

} // namespace WingsOfSteel::Test
