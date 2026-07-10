#include "../../PCH.h"
#include "Passes/Debug/VisualizeBuffersPass.h"

#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

VisualizeBuffersPass::VisualizeBuffersPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const VisualizeBuffersPass::ParameterMetadata& VisualizeBuffersPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<VisualizeBuffersPass>();
}

const RenderPassDefinition& VisualizeBuffersPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::VisualizeBuffers,
	    L"VisualizeBuffers_BindingLayout",
	    L"VisualizeBuffers_PipelineState");
	return definition;
}

void VisualizeBuffersPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->SceneColor = builder.CreateUAV(sceneTargets.FinalSceneColor);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);
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
	parameters->PerFrame = passRuntimeServices.PerFrame;
}

void VisualizeBuffersPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	ComputePassUtilities::DispatchSized<VisualizeBuffersPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}
