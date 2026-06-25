#pragma once

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

namespace RayTracingScenePassBinding
{
	inline bool CanUseSceneTlas(
	    const RayTracingPassCapabilities& capabilities,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		return RayTracingPassCapabilityQuery::CanUseSceneTlas(capabilities, accessMode);
	}

	inline bool FrameUsesSceneTlasAccessMode(
	    const FrameContext& frame,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		return frame.rayTracingScene.HasBoundTlas() && frame.rayTracingScene.TlasShaderAccessMode == accessMode;
	}

	template <typename TParameterInstance>
	bool BindSceneTlas(
	    FrameGraphBuilder& builder,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    RayTracingSceneTlasShaderAccessMode accessMode,
	    TParameterInstance& parameters)
	{
		if (accessMode != RayTracingSceneTlasShaderAccessMode::Descriptor)
		{
			return false;
		}

		parameters->SceneTlas = builder.Read(sceneTlas);
		return true;
	}
}
