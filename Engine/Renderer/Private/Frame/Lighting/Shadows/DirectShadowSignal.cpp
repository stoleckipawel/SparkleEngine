#include "../../../PCH.h"
#include "Frame/Lighting/Shadows/DirectShadowSignal.h"

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& descriptorParameters = builder.AllocParameters<DirectShadowSignalPass::Parameters>();
	descriptorParameters->ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
	descriptorParameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
	descriptorParameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
	descriptorParameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
	descriptorParameters->DirectionalLights = builder.CreateSRV(externalResources.Scene.Lighting.DirectionalLights);
	descriptorParameters->PointLights = builder.CreateSRV(externalResources.Scene.Lighting.PointLights);
	descriptorParameters->SpotLights = builder.CreateSRV(externalResources.Scene.Lighting.SpotLights);
	descriptorParameters->RectLights = builder.CreateSRV(externalResources.Scene.Lighting.RectLights);
	descriptorParameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	descriptorParameters->RayTracingHitVertices = builder.CreateSRV(externalResources.Scene.RayTracing.Vertices);
	descriptorParameters->RayTracingHitIndices = builder.CreateSRV(externalResources.Scene.RayTracing.Indices);
	descriptorParameters->RayTracingHitInstances = builder.CreateSRV(externalResources.Scene.RayTracing.Instances);
	descriptorParameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.Scene.RayTracing.Materials);
	descriptorParameters->SceneTlas = builder.Read(sceneTlas);
	builder.Dispatch<DirectShadowSignalPass>(descriptorParameters);
}
