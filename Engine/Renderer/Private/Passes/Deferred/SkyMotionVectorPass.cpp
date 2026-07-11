#include "../../PCH.h"
#include "Passes/Deferred/SkyMotionVectorPass.h"

#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SkyMotionVectorPass::SkyMotionVectorPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const SkyMotionVectorPass::ParameterMetadata& SkyMotionVectorPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<SkyMotionVectorPass>();
}

const RenderPassDefinition& SkyMotionVectorPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::SkyMotionVector,
	    L"SkyMotionVector_BindingLayout",
	    L"SkyMotionVector_PipelineState");
	return definition;
}

void SkyMotionVectorPass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle motionVector,
    ParameterInstance& parameters)
{
	parameters->GBufferDeviceZ = builder.CreateSRV(deviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(motionVector);
}

void SkyMotionVectorPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->PerFrame = context.RuntimeServices.PerFrame;
	parameters->PerView = context.Frame.mainView.perViewData;
	parameters->PerTemporal = context.Frame.mainView.perTemporalData;
	ComputePassUtilities::DispatchSized<SkyMotionVectorPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}
