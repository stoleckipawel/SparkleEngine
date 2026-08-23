#ifndef SPARKLE_DIRECT_SHADOW_SIGNAL_COMMON_HLSLI
#define SPARKLE_DIRECT_SHADOW_SIGNAL_COMMON_HLSLI

#include "/Engine/Resources/ViewCameraUniformData.hlsli"

#include "/Engine/Lighting/DirectLightReservoir.hlsli"
#include "/Engine/Passes/GBuffer/GBufferUtils.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowSignalPacking.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowVisibility.hlsli"

RWTexture2D<float4> ShadowVisibilitySignal;
Texture2D<float4> CurrentReservoirSample;
Texture2D<float4> CurrentReservoirWeight;

void EvaluateDirectShadowSignal(uint3 dispatchThreadId)
{
	uint width = 0;
	uint height = 0;
	ShadowVisibilitySignal.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float sceneDepth = LoadSceneDepth(dispatchThreadId.xy);
	if (IsSkyPixel(sceneDepth))
	{
		ShadowVisibilitySignal[dispatchThreadId.xy] =
		    RayTracedShadowSignalPacking::PackShadowSignal(RayTracedShadowSignals::BuildUnshadowedSignal(0.0f));
		return;
	}

	const float3 positionWorld = ReconstructGBufferWorldPosition(dispatchThreadId.xy, sceneDepth, InvViewMTX, InvProjectionMTX);
	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(int3(dispatchThreadId.xy, 0)).xyz);
	const DirectLightReservoir::Reservoir reservoir =
	    DirectLightReservoir::UnpackReservoir(CurrentReservoirSample.Load(int3(dispatchThreadId.xy, 0)),
	                                          CurrentReservoirWeight.Load(int3(dispatchThreadId.xy, 0)));
	if (!DirectLightReservoir::IsValid(reservoir))
	{
		ShadowVisibilitySignal[dispatchThreadId.xy] =
		    RayTracedShadowSignalPacking::PackShadowSignal(RayTracedShadowSignals::BuildUnshadowedSignal(0.0f));
		return;
	}

	const LightSampling::DirectLightSample lightSample = DirectLightReservoir::ReplayLightSample(reservoir, positionWorld);
	const ShadowVisibilitySample shadowSignal =
	    RayTracedShadowVisibility::TraceDirectLightSample(positionWorld,
	                                                      normalWorld,
	                                                      lightSample,
	                                                      DirectLightSampling::CastsShadow(reservoir.Candidate.Light));

	ShadowVisibilitySignal[dispatchThreadId.xy] = RayTracedShadowSignalPacking::PackShadowSignal(shadowSignal);
}

#endif
