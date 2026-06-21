#include "../../PCH.h"
#include "Frame/Lighting/DirectLighting.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Deferred/DirectLightingPass.h"
#include "Passes/Core/ShaderPass.h"
#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"

namespace DirectLightingFramePasses
{
	bool UsesNoRayQuery(const FrameContext& frame) noexcept
	{
		return !frame.rayTracingScene.HasTraceableInstances();
	}

	bool UsesDescriptorSceneTlas(const FrameContext& frame) noexcept
	{
		return frame.rayTracingScene.HasTraceableInstances() &&
		       frame.rayTracingScene.TlasShaderAccessMode == RayTracingSceneTlasShaderAccessMode::Descriptor;
	}

	bool UsesShaderDeviceAddressSceneTlas(const FrameContext& frame) noexcept
	{
		return frame.rayTracingScene.HasTraceableInstances() &&
		       frame.rayTracingScene.TlasShaderAccessMode == RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress;
	}
}  // namespace DirectLightingFramePasses

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& noRayParameters = builder.AllocPassParameters<DirectLightingNoRayQueryPass>();
	DirectLightingNoRayQueryPass::DeclareResources(builder, lighting, gbuffer, noRayParameters);
	builder.AddPass(
	    DirectLightingNoRayQueryPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&noRayParameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!DirectLightingFramePasses::UsesNoRayQuery(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<DirectLightingNoRayQueryPass::Parameters>::Setup(
		        resourceBuilder,
		        noRayParameters,
		        DirectLightingNoRayQueryPass::PassName);
	    },
	    [&noRayParameters](PassExecutionContext& context)
	    {
		    if (!DirectLightingFramePasses::UsesNoRayQuery(context.Frame))
		    {
			    return;
		    }

		    const DirectLightingNoRayQueryPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingNoRayQueryPass>());
		    pass.Execute(context, noRayParameters);
	    });

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
