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
	const auto bindCommonParameters = [&](auto& parameters)
	{
		parameters->ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
		parameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
		parameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
		parameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
		parameters->DirectionalLights = builder.CreateSRV(externalResources.Scene.Lighting.DirectionalLights);
		parameters->PointLights = builder.CreateSRV(externalResources.Scene.Lighting.PointLights);
		parameters->SpotLights = builder.CreateSRV(externalResources.Scene.Lighting.SpotLights);
		parameters->RectLights = builder.CreateSRV(externalResources.Scene.Lighting.RectLights);
	};
	const auto bindRayQueryParameters = [&](auto& parameters)
	{
		bindCommonParameters(parameters);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->RayTracingHitVertices = builder.CreateSRV(externalResources.Scene.RayTracing.Vertices);
		parameters->RayTracingHitIndices = builder.CreateSRV(externalResources.Scene.RayTracing.Indices);
		parameters->RayTracingHitInstances = builder.CreateSRV(externalResources.Scene.RayTracing.Instances);
		parameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.Scene.RayTracing.Materials);
	};

	auto& noRayParameters = builder.AllocParameters<DirectShadowSignalNoRayQueryPass::Parameters>();
	bindCommonParameters(noRayParameters);
	LightingRayTracingPasses::DispatchNoRayQuery<DirectShadowSignalNoRayQueryPass>(builder, noRayParameters);

	auto& descriptorParameters = builder.AllocParameters<DirectShadowSignalPass::Parameters>();
	bindRayQueryParameters(descriptorParameters);
	descriptorParameters->SceneTlas = builder.Read(sceneTlas);
	LightingRayTracingPasses::DispatchSceneTlas<DirectShadowSignalPass>(
	    builder,
	    descriptorParameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);

	auto& addressParameters = builder.AllocParameters<DirectShadowSignalDeviceAddressPass::Parameters>();
	bindRayQueryParameters(addressParameters);
	LightingRayTracingPasses::DispatchSceneTlas<DirectShadowSignalDeviceAddressPass>(
	    builder,
	    addressParameters,
	    RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress);
}
