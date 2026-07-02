#include "../../PCH.h"
#include "Frame/Lighting/DirectLighting.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightingPass.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    FrameGraphTextureHandle shadowVisibilitySignal)
{
	auto& noRayParameters = builder.AllocPassParameters<DirectLightingNoRayQueryPass>();
	DirectLightingNoRayQueryPass::DeclareResources(builder, lighting, gbuffer, shadowVisibilitySignal, noRayParameters);
	LightingRayTracingPasses::AddNoRayQueryComputePass<DirectLightingNoRayQueryPass>(builder, noRayParameters);

	auto& descriptorParameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, shadowVisibilitySignal, descriptorParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectLightingPass>(
	    builder,
	    descriptorParameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);

	auto& addressParameters = builder.AllocPassParameters<DirectLightingDeviceAddressPass>();
	DirectLightingDeviceAddressPass::DeclareResources(builder, lighting, gbuffer, shadowVisibilitySignal, addressParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectLightingDeviceAddressPass>(
	    builder,
	    addressParameters,
	    RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress);
}
