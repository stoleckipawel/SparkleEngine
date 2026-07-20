#pragma once

namespace ECS
{
	struct LocalTransform;
	struct OscillatingMotion;

	class OscillatingMeshMotionSystem final
	{
	  public:
		static void Apply(const OscillatingMotion& motion, float timeSeconds, bool useLanes, LocalTransform& transform) noexcept;
	};
}
