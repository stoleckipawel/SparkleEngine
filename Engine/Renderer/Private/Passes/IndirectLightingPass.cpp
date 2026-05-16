#include "../PCH.h"
#include "Passes/IndirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "FrameGraph/Execution/RenderGraphPassContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cassert>

const IndirectLightingPass::ParameterMetadata& IndirectLightingPass::GetParameterMetadata() noexcept
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

ShaderPackageDefinition IndirectLightingPass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{.PackageId = PassName, .BindingLayoutId = PassName, .ExpectedStages = ShaderStageMask::Compute};
}

void IndirectLightingPass::DeclareResources(FrameGraphBuilder& builder, const LightingTargets& lighting, ParameterInstance& parameters)
{
	parameters->IndirectDiffuse = builder.CreateUAV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(lighting.IndirectSpecular);
	parameters->IndirectSubsurface = builder.CreateUAV(lighting.IndirectSubsurface);
}

void IndirectLightingPass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.IndirectLighting.Execute");

	const IndirectLightingPassRuntime& runtime = context.Runtime.GetPassRuntime<IndirectLightingPass>();
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<IndirectLightingPass>(
	    context.Graph,
	    context.Commands,
	    context.Runtime.HardwareInterface,
	    runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}
