#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cassert>
#include <cstdint>
#include <string_view>

namespace ComputePassOperations
{
	template <typename TPass>
	const typename TPass::ParameterMetadata& BuildParameterMetadata()
	{
		static const typename TPass::ParameterMetadata metadata = []
		{
			const typename TPass::ParameterMetadata localMetadata =
			    ShaderParameterStructBuilder<typename TPass::Parameters>::BuildMetadata(TPass::PassName);
			const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Compute, TPass::PassName);
			assert(valid);
			return localMetadata;
		}();

		return metadata;
	}

	RenderPassDefinition BuildDefinition(
	    const char* passName,
	    std::string_view packageId,
	    const wchar_t* bindingLayoutName,
	    const wchar_t* pipelineName,
	    CookedShaderPackageFeatureFlags requiredFeatures = CookedShaderPackageFeatureFlags::None);

	template <typename TPass>
	bool Dispatch(
	    PassExecutionContext& context,
	    const ComputePassPipelineRuntime& runtime,
	    const typename TPass::ParameterInstance& parameters,
	    const ComputeDispatchDesc& dispatch)
	{
		const bool valid = parameters.Sync();
		assert(valid);
		const bool dispatched = ShaderPassOperations::DispatchComputePassWithRuntime<TPass>(
		    context.Resources,
		    context.Commands,
		    context.Runtime.HardwareInterface,
		    runtime,
		    parameters,
		    dispatch,
		    TPass::PassName);
		assert(dispatched);
		return dispatched;
	}

	template <typename TPass>
	bool DispatchSized(
	    PassExecutionContext& context,
	    const ComputePassPipelineRuntime& runtime,
	    const typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		const ComputeDispatchDesc dispatch{
		    MathUtils::DivideRoundUp(outputWidth, TPass::ThreadGroupSizeX),
		    MathUtils::DivideRoundUp(outputHeight, TPass::ThreadGroupSizeY),
		    1};
		return Dispatch<TPass>(context, runtime, parameters, dispatch);
	}
}
