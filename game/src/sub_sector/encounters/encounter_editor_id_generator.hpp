#pragma once

#include <cstdint>

namespace WingsOfSteel::TheBrightestStar
{

class EncounterEditorIdGenerator
{
public:
    EncounterEditorIdGenerator(uint32_t startId = 1)
    {
        m_Id = startId;
    }

    static uint32_t GenerateId();

private:
    static uint32_t m_Id;
};

inline uint32_t EncounterEditorIdGenerator::m_Id = 1;

inline uint32_t EncounterEditorIdGenerator::GenerateId()
{
    return m_Id++;
}

} // namespace WingsOfSteel::TheBrightestStar

