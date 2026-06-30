#include "../../PCH.h"
#include "Frame/Lighting/DirectLighting.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Deferred/DirectLightingPass.h"
#include "Passes/Core/ShaderPass.h"

namespace DirectLightingFramePasses
{
	bool UsesNoRayQuery(const FrameContext& frame) noexcept
	{
		return !frame.rayTracingScene.HasTraceableInstances();
	}

	bool UsesSceneTlasAccessMode(
	    const FrameContext& frame,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		return frame.rayTracingScene.HasTraceableInstances() &&
		       RayTracingScenePassBinding::FrameUsesSceneTlasAccessMode(frame, accessMode);
	}
}  // namespace DirectLightingFramePasses

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    FrameGraphTextureHandle shadowVisibilitySignal)
{
	auto& noRayParameters = builder.AllocPassParameters<DirectLightingNoRayQueryPass>();
	DirectLightingNoRayQueryPass::DeclareResources(builder, lighting, gbuffer, shadowVisibilitySignal, noRayParameters);
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
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, shadowVisibilitySignal, descriptorParameters);
	builder.AddPass(
	    DirectLightingPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&descriptorParameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!DirectLightingFramePasses::UsesSceneTlasAccessMode(frame, RayTracingSceneTlasShaderAccessMode::Descriptor))
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
		    if (!DirectLightingFramePasses::UsesSceneTlasAccessMode(context.Frame, RayTracingSceneTlasShaderAccessMode::Descriptor))
		    {
			    return;
		    }

		    const DirectLightingPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingPass>());
		    pass.Execute(context, descriptorParameters);
	    });

	auto& addressParameters = builder.AllocPassParameters<DirectLightingDeviceAddressPass>();
	DirectLightingDeviceAddressPass::DeclareResources(builder, lighting, gbuffer, shadowVisibilitySignal, addressParameters);
	builder.AddPass(
	    DirectLightingDeviceAddressPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&addressParameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!DirectLightingFramePasses::UsesSceneTlasAccessMode(frame, RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress))
		    {
			    return;
		    }

		    ComputeShaderPass<DirectLightingDeviceAddressPass::Parameters>::Setup(
		        resourceBuilder,
		        addressParameters,
		        DirectLightingDeviceAddressPass::PassName);
	    },
	    [&addressParameters](PassExecutionContext& context)
	    {
		    if (!DirectLightingFramePasses::UsesSceneTlasAccessMode(context.Frame, RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress))
		    {
			    return;
		    }

		    const DirectLightingDeviceAddressPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingDeviceAddressPass>());
		    pass.Execute(context, addressParameters);
	    });
}
