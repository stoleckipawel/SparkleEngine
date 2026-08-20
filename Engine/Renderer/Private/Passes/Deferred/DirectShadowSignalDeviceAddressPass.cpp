#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Passes/Deferred/DirectShadowSignalDeviceAddressPass.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectShadowSignalDeviceAddressPass::DirectShadowSignalDeviceAddressPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectShadowSignalDeviceAddressPass::ParameterMetadata& DirectShadowSignalDeviceAddressPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectShadowSignalDeviceAddressPass>();
}

const RenderPassDefinition& DirectShadowSignalDeviceAddressPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignalDeviceAddress,
	    L"DirectShadowSignalDeviceAddress_BindingLayout",
	    L"DirectShadowSignalDeviceAddress_Pipeline",
	    RayTracingShaderFeatureFlags::DeviceAddressRayQuery);
	return definition;
}

void DirectShadowSignalDeviceAddressPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	DirectShadowSignalPassCommon::SetRayQueryParameters(
	    *parameters,
	    context.Frame,
	    context.Frame.view,
	    context.Runtime,
	    context.Frame.preparedScene.gpuBindings->RayTracing.InstanceCount > 0u);
	ComputePassOperations::DispatchSized<DirectShadowSignalDeviceAddressPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}
