#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <SGP4.h>

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

} // namespace WingsOfSteel::Test
