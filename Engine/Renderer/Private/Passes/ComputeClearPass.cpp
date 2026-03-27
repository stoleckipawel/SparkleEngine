#include "../PCH.h"
#include "Renderer/Public/Passes/ComputeClearPass.h"

#include "Renderer/Public/GPU/CommandContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "Renderer/Public/FrameGraph/RenderPassContext.h"
#include "Renderer/Public/GPU/ComputeUtils.h"
#include "Renderer/Public/Passes/PassUtilities.h"
#include "Renderer/Public/Passes/ShaderPass.h"
#include "Renderer/Public/ShaderParameters/PassParameterLayout.h"

#include <cassert>

const ComputeClearPass::ParameterMetadata& ComputeClearPass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Compute, PassName);
		assert(valid);
		return localMetadata;
	}();
	return metadata;
}

const PassParameterLayout& ComputeClearPass::GetParameterLayout() noexcept
{
	return GetParameterMetadata().GetLayout();
}

ShaderSourceDefinition ComputeClearPass::DescribeShader() noexcept
{
	return ShaderSourceDefinition::FromAsset("Passes/Compute/ComputeClearColorCS.hlsl", "main", ShaderStage::Compute);
}

void ComputeClearPass::Execute(
	RenderGraphPassContext& context,
	const ComputeClearPass::ParameterInstance& parameters,
	std::uint32_t width,
	std::uint32_t height) noexcept
{
	const ComputeDispatchDesc dispatch{
	    ComputeUtils::DivideRoundUp(width, ThreadGroupSizeX),
	    ComputeUtils::DivideRoundUp(height, ThreadGroupSizeY),
	    1};
	const ComputeClearPassRuntime& runtime = context.Runtime.GetPassRuntime<ComputeClearPass>();
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<ComputeClearPass>(
	    context.Graph,
	    context.Commands,
	    context.Runtime.DescriptorHeapManager,
	    runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}