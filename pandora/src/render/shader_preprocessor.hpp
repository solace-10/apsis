#pragma once

#include <cstdint>
#include <string>

namespace WingsOfSteel::Pandora
{

class ShaderPreprocessor
{
public:
    static void Initialize();
    static std::string Execute(const std::string& source);
    static uint32_t ResolveLineNumber(uint32_t lineNumber);
};

} // namespace WingsOfSteel::Pandora