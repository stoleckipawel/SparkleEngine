#pragma once

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ShaderPass.h"

namespace LightingRayTracingPasses
{
	inline bool UsesNoRayQuery(const FrameContext& frame) noexcept
	{
		return !frame.rayTracingScene.HasTraceableInstances();
	}

	inline bool UsesSceneTlasAccessMode(
	    const FrameContext& frame,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		return frame.rayTracingScene.HasTraceableInstances() && frame.rayTracingScene.HasBoundTlas() &&
		       frame.rayTracingScene.TlasShaderAccessMode == accessMode;
	}

	template <typename TPass>
	void AddNoRayQueryComputePass(FrameGraphBuilder& builder, typename TPass::ParameterInstance& parameters)
	{
		builder.AddPass(
		    TPass::PassName,
		    EFrameGraphPassFlags::Compute,
		    [&parameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
		    {
			    if (!UsesNoRayQuery(frame))
			    {
				    return;
			    }

			    ComputeShaderPass<typename TPass::Parameters>::Setup(
			        resourceBuilder,
			        parameters,
			        TPass::PassName);
		    },
		    [&parameters](PassExecutionContext& context)
		    {
			    if (!UsesNoRayQuery(context.Frame))
			    {
				    return;
			    }

			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, parameters);
		    });
	}

	template <typename TPass>
	void AddSceneTlasComputePass(
	    FrameGraphBuilder& builder,
	    typename TPass::ParameterInstance& parameters,
	    RayTracingSceneTlasShaderAccessMode accessMode)
	{
		builder.AddPass(
		    TPass::PassName,
		    EFrameGraphPassFlags::Compute,
		    [&parameters, accessMode](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
		    {
			    if (!UsesSceneTlasAccessMode(frame, accessMode))
			    {
				    return;
			    }

			    ComputeShaderPass<typename TPass::Parameters>::Setup(
			        resourceBuilder,
			        parameters,
			        TPass::PassName);
		    },
		    [&parameters, accessMode](PassExecutionContext& context)
		    {
			    if (!UsesSceneTlasAccessMode(context.Frame, accessMode))
			    {
				    return;
			    }

			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, parameters);
		    });
	}
}
