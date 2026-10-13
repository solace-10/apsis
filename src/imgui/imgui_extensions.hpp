#pragma once

#include <imgui.h>
#include <webgpu/webgpu_cpp.h>

namespace ImGui
{
    
// Wrapper for ImGui::Image that accepts wgpu::TextureView directly
inline void Image(const wgpu::TextureView& textureView, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0))
{
    ::ImGui::Image(reinterpret_cast<ImTextureID>(textureView.Get()), size, uv0, uv1, tint_col, border_col);
}

} // namespace ImGui
