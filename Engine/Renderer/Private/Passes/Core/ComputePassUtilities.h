#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cassert>
#include <cstdint>
#include <string_view>

namespace ComputePassUtilities
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

	inline RenderPassDefinition BuildDefinition(
	    const char* passName,
	    const char* packageDeclarationName,
	    std::string_view packageId,
	    const wchar_t* bindingLayoutName,
	    const wchar_t* pipelineStateName)
	{
		return RenderPassDefinition{
		    .PassName = passName,
		    .PackageDeclarationName = packageDeclarationName,
		    .ShaderPackage = ShaderPackageDefinition{
		        .PackageId = packageId.data(),
		        .BindingLayoutId = packageId.data(),
		        .ExpectedStages = ShaderStageMask::Compute},
		    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
		    .BindingLayoutDebugName = bindingLayoutName,
		    .PipelineStateDebugName = pipelineStateName};
	}

	template <typename TPass>
	bool Dispatch(
	    PassExecutionContext& context,
	    const ComputePassPipelineRuntime& runtime,
	    typename TPass::ParameterInstance& parameters,
	    const ComputeDispatchDesc& dispatch)
	{
		const bool valid = parameters.Sync();
		assert(valid);
		const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<TPass>(
		    context.Resources,
		    context.Commands,
		    context.RuntimeServices.HardwareInterface,
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
	    typename TPass::ParameterInstance& parameters,
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
