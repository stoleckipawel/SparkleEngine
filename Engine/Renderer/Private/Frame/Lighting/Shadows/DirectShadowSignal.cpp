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
		parameters->DirectionalLights = builder.CreateSRV(externalResources.DirectionalLights);
		parameters->PointLights = builder.CreateSRV(externalResources.PointLights);
		parameters->SpotLights = builder.CreateSRV(externalResources.SpotLights);
		parameters->RectLights = builder.CreateSRV(externalResources.RectLights);
	};
	const auto bindRayQueryParameters = [&](auto& parameters)
	{
		bindCommonParameters(parameters);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->RayTracingHitVertices = builder.CreateSRV(externalResources.RayTracingHitVertices);
		parameters->RayTracingHitIndices = builder.CreateSRV(externalResources.RayTracingHitIndices);
		parameters->RayTracingHitInstances = builder.CreateSRV(externalResources.RayTracingHitInstances);
		parameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.RayTracingHitMaterials);
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
