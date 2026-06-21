#include "../PCH.h"
#include "Frame/RTIndirectSpecular.h"

#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/RTIndirectSpecularPass.h"
#include "Passes/ShaderPass.h"
#include "RayTracing/RenderRayTracingPassServices.h"
#include "RayTracing/RTIndirectSpecularRuntimeDiagnostics.h"
#include "RayTracing/RTIndirectSpecularSettings.h"
#include "RayTracing/RayTracingSceneTlasShaderAccessMode.h"

namespace RTIndirectSpecularFramePasses
{
	bool UsesDescriptorSceneTlas(const FrameContext& frame) noexcept
	{
		return frame.rayTracingScene.TlasShaderAccessMode == RayTracingSceneTlasShaderAccessMode::Descriptor;
	}
}  // namespace RTIndirectSpecularFramePasses

void AddRTIndirectSpecularPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<RTIndirectSpecularPass>();
	RTIndirectSpecularPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, parameters);
	builder.AddPass(
	    RTIndirectSpecularPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&parameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!RTIndirectSpecularFramePasses::UsesDescriptorSceneTlas(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<RTIndirectSpecularPass::Parameters>::Setup(
		        resourceBuilder,
		        parameters,
		        RTIndirectSpecularPass::PassName);
	    },
	    [&parameters](PassExecutionContext& context)
	    {
		    if (!RTIndirectSpecularFramePasses::UsesDescriptorSceneTlas(context.Frame))
		    {
			    const RenderRayTracingPassServices* rayTracingServices = context.RuntimeServices.RayTracing;
			    const RTIndirectSpecularSettings settings =
			        rayTracingServices != nullptr && rayTracingServices->IndirectSpecularSettings != nullptr
			            ? *rayTracingServices->IndirectSpecularSettings
			            : BuildRTIndirectSpecularSettingsFromCVars();
			    RTIndirectSpecularRuntimeDiagnostics::Publish(
			        RTIndirectSpecularRuntimeDiagnosticsSnapshot{
			            .Status = settings.Enabled ? RTIndirectSpecularStatusReason::Unsupported : RTIndirectSpecularStatusReason::Disabled,
			            .Enabled = settings.Enabled,
			            .SampleMode = settings.SampleMode,
			            .DebugMode = settings.DebugMode,
			            .MaxDistance = settings.MaxDistance,
			            .HitDataAvailable = context.Frame.rtIndirectSpecularHitData.IsValid() && context.Frame.meshInstances.IsValid(),
			            .HitInstanceCount = context.Frame.rtIndirectSpecularHitData.GetInstanceCount(),
			            .HitMaterialCount = context.Frame.rtIndirectSpecularHitData.GetMaterialCount()});
			    return;
		    }

		    const RTIndirectSpecularPass pass(context.RuntimeServices.GetPassRuntime<RTIndirectSpecularPass>());
		    pass.Execute(context, parameters);
	    });
}
