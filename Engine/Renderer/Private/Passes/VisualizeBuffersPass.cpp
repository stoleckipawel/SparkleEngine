#include "../PCH.h"
#include "Passes/VisualizeBuffersPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewContext.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/RenderPassContext.h"
#include "GPU/PassExecutionDiagnostics.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cassert>

const VisualizeBuffersPass::ParameterMetadata& VisualizeBuffersPass::GetParameterMetadata() noexcept
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

ShaderPackageDefinition VisualizeBuffersPass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{
		.PackageId = PassName,
		.BindingLayoutId = PassName,
		.ExpectedStages = ShaderStageMask::Compute};
}

void VisualizeBuffersPass::DeclareResources(
	FrameGraph& frameGraph,
	const SceneTargets& sceneTargets,
	const LightingTargets& lighting,
	const GBufferTargets& gbuffer,
	ParameterInstance& parameters)
{
	parameters->SceneColor = frameGraph.CreateUAV(sceneTargets.SceneColor);
	parameters->DirectDiffuse = frameGraph.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = frameGraph.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = frameGraph.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = frameGraph.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = frameGraph.CreateSRV(lighting.IndirectSpecular);
	parameters->IndirectSubsurface = frameGraph.CreateSRV(lighting.IndirectSubsurface);
	parameters->GBufferBaseColor = frameGraph.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = frameGraph.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = frameGraph.CreateSRV(gbuffer.Material);
	parameters->GBufferEmissive = frameGraph.CreateSRV(gbuffer.Emissive);
	parameters->GBufferSubsurface = frameGraph.CreateSRV(gbuffer.Subsurface);
}

void VisualizeBuffersPass::SetParameters(
	ParameterInstance& parameters,
	const RenderViewContext& viewContext,
	const RenderPassContext& renderPassContext)
{
	(void)viewContext;
	parameters->PerFrame = renderPassContext.HardwareInterface.GetPerFrameConstantData();
	const bool valid = parameters.Sync();
	assert(valid);
}

void VisualizeBuffersPass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.VisualizeBuffers.Execute");

	const VisualizeBuffersPassRuntime& runtime = context.Runtime.GetPassRuntime<VisualizeBuffersPass>();
	SetParameters(parameters, context.Frame.mainView, context.Runtime);
	const ComputeDispatchDesc dispatch{
		MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
		MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
		1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<VisualizeBuffersPass>(
		context.Graph,
		context.Commands,
		context.Runtime.HardwareInterface,
		runtime,
		parameters,
		dispatch,
		PassName);
	assert(dispatched);
}