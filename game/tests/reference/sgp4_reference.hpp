#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <SGP4.h>

#include "components/orbital_elements_component.hpp"
#include "space/sgp4.hpp"

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

// The times a verification case asks to be propagated to, in the order tcppver.out prints them.
//
// The awkward parts are load bearing rather than stylistic. The leading zero is there because the
// published first row is always at the epoch regardless of where the range starts - which is why
// the negative ranges (04632, 09998) still open with one. The step back before the loop is what
// stops a range starting at zero printing its first row twice. And the clamp to the stop time is
// why a range that does not divide evenly by its step still ends exactly on it.
std::vector<double> VerificationTimes(const VerificationCase& verificationCase);

// The mean elements the reference was initialised from, taken back out of the initialised record.
//
// Initialisation does not disturb them: dpper's writes to the elements are guarded on init == 'n',
// and the t = 0 step sgp4init ends with works on copies. So these are still exactly what
// twoline2rv parsed, and feeding them to our own initialisation makes the comparison a
// measurement of the arithmetic rather than of the parsing.
SGP4Elements AsElements(const elsetrec& satrec);

} // namespace WingsOfSteel::Test
