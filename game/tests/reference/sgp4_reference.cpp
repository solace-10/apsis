#include "reference/sgp4_reference.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace WingsOfSteel::Test
{

namespace
{

    // twoline2rv indexes fixed columns up to 68 and writes into them, so it needs a full-width
    // line in a buffer it is allowed to modify. Short lines are padded with spaces rather than
    // left as NULs, so that the "is this column blank?" tests it makes still mean what they say.
    constexpr size_t kTleLineLength = 69;
    constexpr size_t kTleBufferSize = 130;

    void ToLineBuffer(const std::string& line, char (&buffer)[kTleBufferSize])
    {
        if (line.length() >= kTleBufferSize)
        {
            throw std::runtime_error("TLE line is longer than twoline2rv's buffer: " + line);
        }

        std::memset(buffer, ' ', kTleBufferSize);
        std::memcpy(buffer, line.data(), line.length());
        buffer[std::max(line.length(), kTleLineLength)] = '\0';
    }

    std::vector<std::string> ReadLines(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("Failed to open " + path.string());
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line))
        {
            // The files ship with CRLF endings and are read in text mode on every platform we
            // build for, so the carriage return has to come off here rather than being assumed
            // away. It would otherwise land in the middle of twoline2rv's fixed columns.
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            lines.push_back(std::move(line));
        }
        return lines;
    }

} // namespace

std::vector<VerificationCase> LoadVerificationCases(const std::filesystem::path& path)
{
    const std::vector<std::string> lines = ReadLines(path);

    std::vector<VerificationCase> cases;
    for (size_t i = 0; i + 1 < lines.size(); i++)
    {
        if (lines[i].rfind("1 ", 0) != 0 || lines[i + 1].rfind("2 ", 0) != 0)
        {
            continue;
        }

        char line1[kTleBufferSize];
        char line2[kTleBufferSize];
        ToLineBuffer(lines[i], line1);
        ToLineBuffer(lines[i + 1], line2);

        VerificationCase verificationCase;
        SGP4Funcs::twoline2rv(
            line1, line2,
            'v', // Take the propagation range from the end of line 2.
            'e', // Unused by the 'v' typerun; it only selects a prompt for manual runs.
            'i', // Improved operation mode, which is what the published output was produced with.
            wgs72,
            verificationCase.startMinutes, verificationCase.stopMinutes, verificationCase.stepMinutes,
            verificationCase.satrec);

        verificationCase.satnum = verificationCase.satrec.satnum;
        cases.push_back(verificationCase);

        i++; // Line 2 has been consumed.
    }

    return cases;
}

std::vector<VerificationBlock> LoadVerificationOutput(const std::filesystem::path& path)
{
    std::vector<VerificationBlock> blocks;

    for (const std::string& line : ReadLines(path))
    {
        if (line.empty())
        {
            continue;
        }

        std::istringstream stream(line);

        // A block header is "<satnum> xx". Anything else is a row of numbers.
        std::string first;
        std::string second;
        stream >> first >> second;
        if (second == "xx")
        {
            blocks.push_back(VerificationBlock{ .satnum = first });
            continue;
        }

        if (blocks.empty())
        {
            throw std::runtime_error("Verification output has a row before any satellite header: " + line);
        }

        stream.clear();
        stream.seekg(0);

        VerificationStep step;
        stream >> step.tsince
            >> step.position[0] >> step.position[1] >> step.position[2]
            >> step.velocity[0] >> step.velocity[1] >> step.velocity[2];

        if (stream.fail())
        {
            throw std::runtime_error("Failed to parse a verification output row: " + line);
        }

        blocks.back().steps.push_back(step);
    }

    return blocks;
}

OrbitalElementsComponent AsComponent(const elsetrec& satrec)
{
    constexpr double kRadiansToDegrees = 180.0 / 3.14159265358979323846;
    constexpr double kRadiansPerMinuteToRevsPerDay = 1440.0 / (2.0 * 3.14159265358979323846);

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    double seconds = 0.0;
    SGP4Funcs::invjday_SGP4(satrec.jdsatepoch, satrec.jdsatepochF, year, month, day, hour, minute, seconds);

    char epoch[32];
    std::snprintf(epoch, sizeof(epoch), "%04d-%02d-%02dT%02d:%02d:%09.6f", year, month, day, hour, minute, seconds);

    const Json::Data json = {
        { "norad_id", std::atoi(satrec.satnum) },
        { "epoch", epoch },
        { "mean_motion", satrec.no_kozai * kRadiansPerMinuteToRevsPerDay },
        { "eccentricity", satrec.ecco },
        { "inclination", satrec.inclo * kRadiansToDegrees },
        { "raan", satrec.nodeo * kRadiansToDegrees },
        { "arg_of_pericenter", satrec.argpo * kRadiansToDegrees },
        { "mean_anomaly", satrec.mo * kRadiansToDegrees },
        { "bstar", satrec.bstar }
    };

    OrbitalElementsComponent component;
    component.Deserialize(nullptr, json);
    return component;
}

std::vector<double> VerificationTimes(const VerificationCase& verificationCase)
{
    std::vector<double> times;
    times.push_back(0.0);

    double tsince = verificationCase.startMinutes;
    if (std::abs(tsince) > 1.0e-8)
    {
        tsince -= verificationCase.stepMinutes;
    }

    while (tsince < verificationCase.stopMinutes)
    {
        tsince += verificationCase.stepMinutes;
        if (tsince > verificationCase.stopMinutes)
        {
            tsince = verificationCase.stopMinutes;
        }

        times.push_back(tsince);
    }

    return times;
}

SGP4Elements AsElements(const elsetrec& satrec)
{
    SGP4Elements elements;
    elements.bstar = satrec.bstar;
    elements.ecco = satrec.ecco;
    elements.argpo = satrec.argpo;
    elements.inclo = satrec.inclo;
    elements.mo = satrec.mo;
    elements.nodeo = satrec.nodeo;
    elements.noKozai = satrec.no_kozai;

    // The reference's own definition of the epoch it initialises from. JD 2433281.5 is
    // 1950 January 0.0.
    elements.epochDaysSince1950 = (satrec.jdsatepoch + satrec.jdsatepochF) - 2433281.5;

    return elements;
}

} // namespace WingsOfSteel::Test
