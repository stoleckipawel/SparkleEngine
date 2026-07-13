#include "../../../PCH.h"
#include "Frame/Lighting/Shadows/DirectShadowSignal.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "Frame/Core/FrameAssembly.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectShadowSignalDeviceAddressPass.h"
#include "Passes/Deferred/DirectShadowSignalNoRayQueryPass.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& noRayParameters = builder.AllocPassParameters<DirectShadowSignalNoRayQueryPass>();
	DirectShadowSignalNoRayQueryPass::DeclareResources(
	    builder,
	    sceneTargets.SceneDepth,
	    shadowSignals,
	    externalResources.DirectionalLights,
	    externalResources.PointLights,
	    externalResources.SpotLights,
	    externalResources.RectLights,
	    noRayParameters);
	LightingRayTracingPasses::AddNoRayQueryComputePass<DirectShadowSignalNoRayQueryPass>(builder, noRayParameters);

	auto& descriptorParameters = builder.AllocPassParameters<DirectShadowSignalPass>();
	DirectShadowSignalPass::DeclareResources(
	    builder,
	    sceneTargets.SceneDepth,
	    gbuffer,
	    sceneTlas,
	    shadowSignals,
	    externalResources.DirectionalLights,
	    externalResources.PointLights,
	    externalResources.SpotLights,
	    externalResources.RectLights,
	    externalResources.RayTracingHitVertices,
	    externalResources.RayTracingHitIndices,
	    externalResources.RayTracingHitInstances,
	    externalResources.RayTracingHitMaterials,
	    descriptorParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectShadowSignalPass>(
	    builder,
	    descriptorParameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);

	auto& addressParameters = builder.AllocPassParameters<DirectShadowSignalDeviceAddressPass>();
	DirectShadowSignalDeviceAddressPass::DeclareResources(
	    builder,
	    sceneTargets.SceneDepth,
	    gbuffer,
	    shadowSignals,
	    externalResources.DirectionalLights,
	    externalResources.PointLights,
	    externalResources.SpotLights,
	    externalResources.RectLights,
	    externalResources.RayTracingHitVertices,
	    externalResources.RayTracingHitIndices,
	    externalResources.RayTracingHitInstances,
	    externalResources.RayTracingHitMaterials,
	    addressParameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<DirectShadowSignalDeviceAddressPass>(
	    builder,
	    addressParameters,
	    RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress);
}
