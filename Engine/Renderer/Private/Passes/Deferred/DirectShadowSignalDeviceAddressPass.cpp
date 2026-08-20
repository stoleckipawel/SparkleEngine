#include "../../PCH.h"

#include "Passes/Deferred/DirectShadowSignalDeviceAddressPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
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

void DirectShadowSignalDeviceAddressPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<DirectShadowSignalDeviceAddressPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}
