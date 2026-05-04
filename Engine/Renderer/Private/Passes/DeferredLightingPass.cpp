#include "../PCH.h"
#include "Passes/DeferredLightingPass.h"

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

const DeferredLightingPass::ParameterMetadata& DeferredLightingPass::GetParameterMetadata() noexcept
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

ShaderPackageDefinition DeferredLightingPass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{
	    .PackageId = PassName,
	    .VariantId = "Default",
	    .BindingLayoutId = PassName,
	    .ExpectedStages = ShaderStageMask::Compute};
}

void DeferredLightingPass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.DeferredLighting.Execute");

	const DeferredLightingPassRuntime& runtime = context.Runtime.GetPassRuntime<DeferredLightingPass>();
	SetParameters(parameters, context.Frame.mainView, context.Runtime);

	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<DeferredLightingPass>(
	    context.Graph,
	    context.Commands,
	    context.Runtime.HardwareInterface,
	    runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}

void DeferredLightingPass::DeclareResources(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const GBufferTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->SceneColor = frameGraph.CreateUAV(sceneTargets.SceneColor);
	parameters->GBufferBaseColor = frameGraph.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = frameGraph.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = frameGraph.CreateSRV(gbuffer.Material);
	parameters->GBufferEmissive = frameGraph.CreateSRV(gbuffer.Emissive);
	parameters->GBufferSubsurface = frameGraph.CreateSRV(gbuffer.Subsurface);
	parameters->GBufferDeviceZ = frameGraph.CreateSRV(gbuffer.DeviceZ);
}

void DeferredLightingPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewContext& viewContext,
    const RenderPassContext& renderPassContext)
{
	parameters->PerFrame = renderPassContext.HardwareInterface.GetPerFrameConstantData();
	parameters->PerView = viewContext.perViewData;
	const bool valid = parameters.Sync();
	assert(valid);
}
