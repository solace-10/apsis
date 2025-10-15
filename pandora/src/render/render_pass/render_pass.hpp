#pragma once

#include <string>

#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(RenderPass);
class RenderPass
{
public:
    RenderPass(const std::string& name)
    : m_Name(name)
    {}

    virtual ~RenderPass() {}
    virtual void Render(wgpu::CommandEncoder& encoder) = 0;

    const std::string& GetName() const { return m_Name; }

private:
    std::string m_Name;
};

} // namespace WingsOfSteel