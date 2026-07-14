#include "../../PCH.h"
#include "Frame/Lighting/DirectLighting.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightingPass.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& parameters = builder.AllocParameters<DirectLightingPass::Parameters>();
	parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
	parameters->ShadowVisibilitySignal = builder.CreateSRV(shadowSignals.Visibility);
	parameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
	parameters->DirectionalLights = builder.CreateSRV(externalResources.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(externalResources.PointLights);
	parameters->SpotLights = builder.CreateSRV(externalResources.SpotLights);
	parameters->RectLights = builder.CreateSRV(externalResources.RectLights);
	builder.Dispatch<DirectLightingPass>(parameters);
}
