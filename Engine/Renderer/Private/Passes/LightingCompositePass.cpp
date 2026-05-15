#include "../PCH.h"
#include "Passes/LightingCompositePass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewContext.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/RenderPassContext.h"
#include "GPU/PassExecutionDiagnostics.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "FrameGraph/Execution/RenderGraphPassContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cassert>

const LightingCompositePass::ParameterMetadata& LightingCompositePass::GetParameterMetadata() noexcept
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

ShaderPackageDefinition LightingCompositePass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{.PackageId = PassName, .BindingLayoutId = PassName, .ExpectedStages = ShaderStageMask::Compute};
}

void LightingCompositePass::DeclareResources(
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
	parameters->GBufferDeviceZ = frameGraph.CreateSRV(gbuffer.DeviceZ);
}

void LightingCompositePass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewContext& viewContext,
    const RenderPassContext& renderPassContext)
{
	parameters->PerFrame = renderPassContext.HardwareInterface.GetPerFrameConstantData();
	parameters->PerView = viewContext.perViewData;
	const bool valid = parameters.Sync();
	assert(valid);
}

void LightingCompositePass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.LightingComposite.Execute");

	const LightingCompositePassRuntime& runtime = context.Runtime.GetPassRuntime<LightingCompositePass>();
	SetParameters(parameters, context.Frame.mainView, context.Runtime);
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<LightingCompositePass>(
	    context.Graph,
	    context.Commands,
	    context.Runtime.HardwareInterface,
	    runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}