#include "PCH.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"

namespace LightingRayTracingPasses
{
	bool UsesNoRayQuery(const FrameContext& frame) noexcept
	{
		return !frame.rayTracingScene.HasTraceableInstances();
	}

	bool UsesSceneTlasAccessMode(
	    const FrameContext& frame,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		return frame.rayTracingScene.HasTraceableInstances() && frame.rayTracingScene.HasBoundTlas() &&
		       frame.rayTracingScene.TlasShaderAccessMode == accessMode;
	}
}
