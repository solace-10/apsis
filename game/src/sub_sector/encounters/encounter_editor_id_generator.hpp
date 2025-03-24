#pragma once

#include <cstdint>

namespace WingsOfSteel::TheBrightestStar
{

using BlueprintId = uint32_t;
static constexpr BlueprintId InvalidBlueprintId = 0;

class EncounterEditorIdGenerator
{
public:
    EncounterEditorIdGenerator(BlueprintId startId = 1)
    {
        m_Id = startId;
    }

    static BlueprintId GenerateId();

private:
    static BlueprintId m_Id;
};

inline BlueprintId EncounterEditorIdGenerator::m_Id = 1;

inline BlueprintId EncounterEditorIdGenerator::GenerateId()
{
    return m_Id++;
}

} // namespace WingsOfSteel::TheBrightestStar

