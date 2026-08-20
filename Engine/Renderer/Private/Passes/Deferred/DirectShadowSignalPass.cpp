#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectShadowSignalPass::DirectShadowSignalPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectShadowSignalPass::ParameterMetadata& DirectShadowSignalPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectShadowSignalPass>();
}

const RenderPassDefinition& DirectShadowSignalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignal,
	    L"DirectShadowSignal_BindingLayout",
	    L"DirectShadowSignal_Pipeline",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void DirectShadowSignalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	DirectShadowSignalPassCommon::SetRayQueryParameters(
	    *parameters,
	    context.Frame,
	    context.Frame.view,
	    context.Runtime,
	    context.Frame.preparedScene.gpuBindings->RayTracing.InstanceCount > 0u);
	ComputePassOperations::DispatchSized<DirectShadowSignalPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}
