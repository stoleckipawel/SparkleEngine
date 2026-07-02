#include "../../../PCH.h"
#include "Frame/Lighting/Shadows/DirectShadowSignal.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const DirectShadowSignalResources& shadowSignals)
{
	auto& noRayParameters = builder.AllocPassParameters<DirectShadowSignalNoRayQueryPass>();
	DirectShadowSignalNoRayQueryPass::DeclareResources(builder, gbuffer, shadowSignals, noRayParameters);
	LightingRayTracingPasses::AddNoRayQueryComputePass<DirectShadowSignalNoRayQueryPass>(builder, noRayParameters);

	auto& descriptorParameters = builder.AllocPassParameters<DirectShadowSignalPass>();
	DirectShadowSignalPass::DeclareResources(builder, gbuffer, sceneTlas, shadowSignals, descriptorParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectShadowSignalPass>(
	    builder,
	    descriptorParameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);

	auto& addressParameters = builder.AllocPassParameters<DirectShadowSignalDeviceAddressPass>();
	DirectShadowSignalDeviceAddressPass::DeclareResources(builder, gbuffer, shadowSignals, addressParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectShadowSignalDeviceAddressPass>(
	    builder,
	    addressParameters,
	    RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress);
}
