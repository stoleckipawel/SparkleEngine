#include "../PCH.h"
#include "Passes/LightingCompositePass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/RenderPassContext.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
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
    FrameGraphBuilder& builder,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->SceneColor = builder.CreateUAV(sceneTargets.SceneColor);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);
	parameters->IndirectSubsurface = builder.CreateSRV(lighting.IndirectSubsurface);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferEmissive = builder.CreateSRV(gbuffer.Emissive);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
}

void LightingCompositePass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const RenderPassContext& renderPassContext)
{
	parameters->PerFrame = renderPassContext.HardwareInterface.GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
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
