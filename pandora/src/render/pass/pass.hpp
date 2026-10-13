#pragma once

#include <string>

#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

// A unit of GPU work recorded into the frame's command encoder.
//
// Deliberately not tied to rendering: a pass opens whatever encoder it needs - render
// or compute - and is also the right place for encoder-level commands such as buffer
// copies, which cannot be issued from within a pass encoder at all.
DECLARE_SMART_PTR(Pass);
class Pass
{
public:
    Pass(const std::string& name)
    : m_Name(name)
    {}

    virtual ~Pass() {}
    virtual void Execute(wgpu::CommandEncoder& encoder) = 0;

    const std::string& GetName() const { return m_Name; }

private:
    std::string m_Name;
};

} // namespace WingsOfSteel
