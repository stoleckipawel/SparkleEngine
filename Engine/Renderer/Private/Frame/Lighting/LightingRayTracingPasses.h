#pragma once

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
namespace LightingRayTracingPasses
{
	bool UsesNoRayQuery(const FrameContext& frame) noexcept;
	bool UsesSceneTlasAccessMode(
	    const FrameContext& frame,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept;

	template <typename TPass>
	void DispatchNoRayQuery(FrameGraphBuilder& builder, typename TPass::ParameterInstance& parameters)
	{
		builder.DispatchIf<TPass>(
		    parameters,
		    [](const FrameContext& frame)
		    {
			    return UsesNoRayQuery(frame);
		    });
	}

	template <typename TPass>
	void DispatchSceneTlas(
	    FrameGraphBuilder& builder,
	    typename TPass::ParameterInstance& parameters,
	    RayTracingSceneTlasShaderAccessMode accessMode)
	{
		builder.DispatchIf<TPass>(
		    parameters,
		    [accessMode](const FrameContext& frame)
		    {
			    return UsesSceneTlasAccessMode(frame, accessMode);
		    });
	}
}
