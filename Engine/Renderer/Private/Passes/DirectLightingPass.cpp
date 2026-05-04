#include "../PCH.h"
#include "Passes/DirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewContext.h"
#include "FrameGraph/FrameGraph.h"
#include "GPU/PassExecutionDiagnostics.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cassert>

const DirectLightingPass::ParameterMetadata& DirectLightingPass::GetParameterMetadata() noexcept
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

ShaderPackageDefinition DirectLightingPass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{
		.PackageId = PassName,
		.VariantId = "Default",
		.BindingLayoutId = PassName,
		.ExpectedStages = ShaderStageMask::Compute};
}

void DirectLightingPass::DeclareResources(
	FrameGraph& frameGraph,
	const LightingTargets& lighting,
	const GBufferTargets& gbuffer,
	ParameterInstance& parameters)
{
	parameters->DirectDiffuse = frameGraph.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = frameGraph.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = frameGraph.CreateUAV(lighting.DirectSubsurface);
	parameters->GBufferNormal = frameGraph.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = frameGraph.CreateSRV(gbuffer.Material);
	parameters->GBufferDeviceZ = frameGraph.CreateSRV(gbuffer.DeviceZ);
}

void DirectLightingPass::SetParameters(
	ParameterInstance& parameters,
	const RenderViewContext& viewContext)
{
	parameters->PerView = viewContext.perViewData;
	const bool valid = parameters.Sync();
	assert(valid);
}

void DirectLightingPass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.DirectLighting.Execute");

	const DirectLightingPassRuntime& runtime = context.Runtime.GetPassRuntime<DirectLightingPass>();
	SetParameters(parameters, context.Frame.mainView);
	const ComputeDispatchDesc dispatch{
		MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
		MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
		1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<DirectLightingPass>(
		context.Graph,
		context.Commands,
		context.Runtime.HardwareInterface,
		runtime,
		parameters,
		dispatch,
		PassName);
	assert(dispatched);
}