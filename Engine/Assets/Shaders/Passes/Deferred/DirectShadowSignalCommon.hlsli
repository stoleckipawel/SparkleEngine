#ifndef SPARKLE_DIRECT_SHADOW_SIGNAL_COMMON_HLSLI
#define SPARKLE_DIRECT_SHADOW_SIGNAL_COMMON_HLSLI

#include "Lighting/DirectLightReservoir.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSignalPacking.hlsli"
#include "RayTracing/Shadows/RayTracedShadowVisibility.hlsli"

RWTexture2D<float4> ShadowVisibilitySignalTexture;
Texture2D<float4> CurrentReservoirSampleTexture;
Texture2D<float4> CurrentReservoirWeightTexture;

void EvaluateDirectShadowSignal(uint3 dispatchThreadId)
{
	uint width = 0;
	uint height = 0;
	ShadowVisibilitySignalTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float sceneDepth = LoadSceneDepth(dispatchThreadId.xy);
	if (IsSkyPixel(sceneDepth))
	{
		ShadowVisibilitySignalTexture[dispatchThreadId.xy] =
		    RayTracedShadowSignalPacking::PackShadowSignal(RayTracedShadowSignals::BuildUnshadowedSignal(0.0f));
		return;
	}

	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, sceneDepth, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(int3(dispatchThreadId.xy, 0)).xyz);
	const DirectLightReservoir::Reservoir reservoir = DirectLightReservoir::UnpackReservoir(
	    CurrentReservoirSampleTexture.Load(int3(dispatchThreadId.xy, 0)),
	    CurrentReservoirWeightTexture.Load(int3(dispatchThreadId.xy, 0)));
	if (!DirectLightReservoir::IsValid(reservoir))
	{
		ShadowVisibilitySignalTexture[dispatchThreadId.xy] =
		    RayTracedShadowSignalPacking::PackShadowSignal(RayTracedShadowSignals::BuildUnshadowedSignal(0.0f));
		return;
	}

	const LightSampling::DirectLightSample lightSample = DirectLightReservoir::ReplayLightSample(reservoir, positionWorld);
	const ShadowVisibilitySignal shadowSignal = RayTracedShadowVisibility::TraceDirectLightSample(
	    positionWorld,
	    normalWorld,
	    lightSample,
	    DirectLightSampling::CastsShadow(reservoir.Candidate.Light));

	ShadowVisibilitySignalTexture[dispatchThreadId.xy] = RayTracedShadowSignalPacking::PackShadowSignal(shadowSignal);
}

#endif
