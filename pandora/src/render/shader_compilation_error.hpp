#pragma once

#include <string>

namespace WingsOfSteel
{

class ShaderCompilationError
{
public:
    ShaderCompilationError(const std::string& message, uint32_t lineNumber, uint32_t linePosition)
        : m_Message(message)
        , m_LineNumber(lineNumber)
        , m_LinePosition(linePosition)
    {
    }

    ~ShaderCompilationError() {}

    inline const std::string& GetMessage() const { return m_Message; }
    inline uint32_t GetLineNumber() const { return m_LineNumber; }
    inline uint32_t GetLinePosition() const { return m_LinePosition; }

private:
    std::string m_Message;
    uint32_t m_LineNumber;
    uint32_t m_LinePosition;
};

} // namespace WingsOfSteel