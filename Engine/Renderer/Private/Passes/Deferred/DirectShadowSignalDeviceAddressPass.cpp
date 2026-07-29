#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalDeviceAddressPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
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
	    context.Frame.mainView,
	    context.Runtime,
	    context.Frame.rayTracingScene.HasTraceableInstances());
	ComputePassOperations::DispatchSized<DirectShadowSignalDeviceAddressPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}
