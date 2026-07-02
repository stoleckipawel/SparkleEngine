#include "../../../PCH.h"
#include "Frame/Lighting/Shadows/DirectShadowSignal.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    FrameGraphTextureHandle shadowVisibilitySignal)
{
	auto& noRayParameters = builder.AllocPassParameters<DirectShadowSignalNoRayQueryPass>();
	DirectShadowSignalNoRayQueryPass::DeclareResources(builder, gbuffer, shadowVisibilitySignal, noRayParameters);
	LightingRayTracingPasses::AddNoRayQueryComputePass<DirectShadowSignalNoRayQueryPass>(builder, noRayParameters);

	auto& descriptorParameters = builder.AllocPassParameters<DirectShadowSignalPass>();
	DirectShadowSignalPass::DeclareResources(builder, gbuffer, sceneTlas, shadowVisibilitySignal, descriptorParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectShadowSignalPass>(
	    builder,
	    descriptorParameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);

	auto& addressParameters = builder.AllocPassParameters<DirectShadowSignalDeviceAddressPass>();
	DirectShadowSignalDeviceAddressPass::DeclareResources(builder, gbuffer, shadowVisibilitySignal, addressParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectShadowSignalDeviceAddressPass>(
	    builder,
	    addressParameters,
	    RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress);
}
