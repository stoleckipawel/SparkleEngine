#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/DirectLightingPass.h"
#include "Passes/ShaderPass.h"
#include "RayTracing/RayTracingSceneTlasShaderAccessMode.h"

namespace DirectLightingFramePasses
{
	bool UsesDescriptorSceneTlas(const FrameContext& frame) noexcept
	{
		return frame.rayTracingScene.TlasShaderAccessMode == RayTracingSceneTlasShaderAccessMode::Descriptor;
	}

	bool UsesShaderDeviceAddressSceneTlas(const FrameContext& frame) noexcept
	{
		return frame.rayTracingScene.TlasShaderAccessMode == RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress;
	}
}  // namespace DirectLightingFramePasses

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& descriptorParameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, descriptorParameters);
	builder.AddPass(
	    DirectLightingPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&descriptorParameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!DirectLightingFramePasses::UsesDescriptorSceneTlas(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<DirectLightingPass::Parameters>::Setup(
		        resourceBuilder,
		        descriptorParameters,
		        DirectLightingPass::PassName);
	    },
	    [&descriptorParameters](PassExecutionContext& context)
	    {
		    if (!DirectLightingFramePasses::UsesDescriptorSceneTlas(context.Frame))
		    {
			    return;
		    }

		    const DirectLightingPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingPass>());
		    pass.Execute(context, descriptorParameters);
	    });

	auto& addressParameters = builder.AllocPassParameters<DirectLightingVulkanAddressPass>();
	DirectLightingVulkanAddressPass::DeclareResources(builder, lighting, gbuffer, addressParameters);
	builder.AddPass(
	    DirectLightingVulkanAddressPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&addressParameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!DirectLightingFramePasses::UsesShaderDeviceAddressSceneTlas(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<DirectLightingVulkanAddressPass::Parameters>::Setup(
		        resourceBuilder,
		        addressParameters,
		        DirectLightingVulkanAddressPass::PassName);
	    },
	    [&addressParameters](PassExecutionContext& context)
	    {
		    if (!DirectLightingFramePasses::UsesShaderDeviceAddressSceneTlas(context.Frame))
		    {
			    return;
		    }

		    const DirectLightingVulkanAddressPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingVulkanAddressPass>());
		    pass.Execute(context, addressParameters);
	    });
}
