#include "../PCH.h"
#include "Passes/ComputeClearPass.h"

#include "GPU/CommandContext.h"
#include "GPU/PassExecutionDiagnostics.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "FrameGraph/RenderPassContext.h"
#include "Core/Public/Math/MathUtils.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

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

ShaderPackageDefinition ComputeClearPass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{
	    .PackageId = PassName,
	    .VariantId = "Default",
	    .BindingLayoutId = PassName,
	    .ExpectedStages = ShaderStageMask::Compute};
}

void ComputeClearPass::Execute(
    RenderGraphPassContext& context,
    const ComputeClearPass::ParameterInstance& parameters,
    std::uint32_t width,
    std::uint32_t height) noexcept
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.ComputeClear.Execute");

	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(width, ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(height, ThreadGroupSizeY),
	    1};
	const ComputeClearPassRuntime& runtime = context.Runtime.GetPassRuntime<ComputeClearPass>();
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<ComputeClearPass>(
	    context.Graph,
	    context.Commands,
	    context.Runtime.HardwareInterface,
	    runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}