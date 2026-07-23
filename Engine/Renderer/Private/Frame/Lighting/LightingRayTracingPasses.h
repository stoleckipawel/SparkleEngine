#pragma once

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ShaderPass.h"

namespace LightingRayTracingPasses
{
	bool UsesNoRayQuery(const FrameContext& frame) noexcept;
	bool UsesSceneTlasAccessMode(
	    const FrameContext& frame,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept;

	template <typename TPass>
	void DispatchNoRayQuery(FrameGraphBuilder& builder, typename TPass::ParameterInstance& parameters)
	{
		builder.Execute(
		    TPass::PassName,
		    EFrameGraphPassKind::Compute,
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
	void DispatchSceneTlas(
	    FrameGraphBuilder& builder,
	    typename TPass::ParameterInstance& parameters,
	    RayTracingSceneTlasShaderAccessMode accessMode)
	{
		builder.Execute(
		    TPass::PassName,
		    EFrameGraphPassKind::Compute,
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
