#include "../PCH.h"
#include "Passes/VisualizeBuffersPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Passes/PassUtilities.h"
#include "Passes/RenderPassDefinition.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include <cassert>

VisualizeBuffersPass::VisualizeBuffersPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

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

const RenderPassDefinition& VisualizeBuffersPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "VisualizeBuffersShaderPackage",
	    .ShaderPackage = ShaderPackageDefinition{
	        .PackageId = RendererShaderPackages::VisualizeBuffers.data(),
	        .BindingLayoutId = RendererShaderPackages::VisualizeBuffers.data(),
	        .ExpectedStages = ShaderStageMask::Compute},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"VisualizeBuffers_BindingLayout",
	    .PipelineStateDebugName = L"VisualizeBuffers_PipelineState"};
	return definition;
}

void VisualizeBuffersPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
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
}

void VisualizeBuffersPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
	const PassRuntimeServices& passRuntimeServices) const
{
	(void) viewData;
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetUploadService().GetPerFrameConstantData();
	const bool valid = parameters.Sync();
	assert(valid);
}

void VisualizeBuffersPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.VisualizeBuffers.Execute");

	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<VisualizeBuffersPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}
