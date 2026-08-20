#include "../../PCH.h"
#include "Passes/Deferred/SkyMotionVectorPass.h"

#include "View/RenderView.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SkyMotionVectorPass::SkyMotionVectorPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const SkyMotionVectorPass::ParameterMetadata& SkyMotionVectorPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<SkyMotionVectorPass>();
}

const RenderPassDefinition& SkyMotionVectorPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::SkyMotionVector,
	    L"SkyMotionVector_BindingLayout",
	    L"SkyMotionVector_Pipeline");
	return definition;
}

void SkyMotionVectorPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->View = context.Frame.view.uniform;
	parameters->ViewCamera = context.Frame.view.cameraUniform;
	parameters->ViewTemporal = context.Frame.view.temporalUniform;
	ComputePassOperations::DispatchSized<SkyMotionVectorPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}
