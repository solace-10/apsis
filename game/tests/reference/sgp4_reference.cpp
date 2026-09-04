#include "reference/sgp4_reference.hpp"

#include <algorithm>
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

} // namespace WingsOfSteel::Test
