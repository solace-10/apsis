#pragma once

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(BulletDebugRender);

DECLARE_SMART_PTR(PhysicsVisualization);
class PhysicsVisualization
{
public:
    PhysicsVisualization() {}

	enum class Mode
	{
		None				= 0,
		Wireframe			= 1 << 0,
		AABB				= 1 << 1,
		Transforms			= 1 << 2,
		RayTests			= 1 << 3,
		ContactPoints		= 1 << 4
	};

	void SetEnabled(Mode mode, bool state);
	bool IsEnabled(Mode mode) const;

private:
    int32_t ToBulletFlag(Mode mode) const;

	int32_t m_BulletDebugMode{0};
	int32_t m_DebugMode{0};
};

} // namespace WingsOfSteel::Pandora