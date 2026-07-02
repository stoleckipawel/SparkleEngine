#include "../../PCH.h"
#include "Passes/Deferred/LightingCompositePass.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

LightingCompositePass::LightingCompositePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const LightingCompositePass::ParameterMetadata& LightingCompositePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<LightingCompositePass>();
}

const RenderPassDefinition& LightingCompositePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    "LightingCompositeShaderPackage",
	    RendererShaderPackages::LightingComposite,
	    L"LightingComposite_BindingLayout",
	    L"LightingComposite_PipelineState");
	return definition;
}

void LightingCompositePass::DeclareResources(
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
	parameters->GBufferEmissive = builder.CreateSRV(gbuffer.Emissive);
}

void LightingCompositePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	ComputePassUtilities::DispatchSized<LightingCompositePass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}
