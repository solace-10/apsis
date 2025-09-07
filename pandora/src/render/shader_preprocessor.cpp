#include "render/shader_preprocessor.hpp"

namespace WingsOfSteel::Pandora
{

static std::string s_PreludeCode = R"(
struct GlobalUniforms
{
    projectionMatrix: mat4x4<f32>,
    viewMatrix: mat4x4<f32>,
    cameraPosition: vec4f,
    time: f32,
    windowWidth: f32,
    windowHeight: f32
};
)";

static uint32_t s_PreludeLineCount = 0;

void ShaderPreprocessor::Initialize()
{
    uint32_t preludeLineCount = 0;
    uint32_t currentLine = 0;

    for (const auto& c : s_PreludeCode)
    {
        if (c == '\n')
        {
            preludeLineCount++;
        }
    }

    s_PreludeLineCount = preludeLineCount;
}

std::string ShaderPreprocessor::Execute(const std::string& source)
{
    std::string result = s_PreludeCode + source;
    return result;
}

uint32_t ShaderPreprocessor::ResolveLineNumber(uint32_t lineNumber)
{
    return lineNumber - s_PreludeLineCount;
}

} // namespace WingsOfSteel::Pandora
