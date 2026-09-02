#ifndef SPARKLE_DIRECT_SHADOW_SIGNAL_COMMON_HLSLI
#define SPARKLE_DIRECT_SHADOW_SIGNAL_COMMON_HLSLI

#include "/Engine/Resources/ViewCameraUniformData.hlsli"

#include "/Engine/Lighting/DirectLightReservoir.hlsli"
#include "/Engine/Passes/GBuffer/GBufferUtils.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowSignalPacking.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowSemantics.hlsli"

RWTexture2D<float4> ShadowVisibilitySignal;
Texture2D<float4> CurrentReservoirSample;
Texture2D<float4> CurrentReservoirWeight;

bool PrepareDirectShadowSignal(uint2 pixelCoord,
                               out bool validPixel,
                               out RayTracedShadowRequest request,
                               out ShadowVisibilitySample immediateSignal)
{
	validPixel = false;
	request = (RayTracedShadowRequest)0;
	immediateSignal = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
	uint width = 0;
	uint height = 0;
	ShadowVisibilitySignal.GetDimensions(width, height);

	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return false;
	}
	validPixel = true;

	const float sceneDepth = LoadSceneDepth(pixelCoord);
	if (IsSkyPixel(sceneDepth))
	{
		return false;
	}

	const float3 positionWorld = ReconstructGBufferWorldPosition(pixelCoord, sceneDepth, InvViewMTX, InvProjectionMTX);
	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(int3(pixelCoord, 0)).xyz);
	const DirectLightReservoir::Reservoir reservoir =
	    DirectLightReservoir::UnpackReservoir(CurrentReservoirSample.Load(int3(pixelCoord, 0)),
	                                          CurrentReservoirWeight.Load(int3(pixelCoord, 0)));
	if (!DirectLightReservoir::IsValid(reservoir))
	{
		return false;
	}

	const LightSampling::DirectLightSample lightSample = DirectLightReservoir::ReplayLightSample(reservoir, positionWorld);
	return RayTracedShadows::BuildDirectLightRequest(positionWorld,
	                                                 normalWorld,
	                                                 lightSample,
	                                                 DirectLightSampling::CastsShadow(reservoir.Candidate.Light),
	                                                 request,
	                                                 immediateSignal);
}

void StoreDirectShadowSignal(uint2 pixelCoord, ShadowVisibilitySample signal)
{
	ShadowVisibilitySignal[pixelCoord] = RayTracedShadowSignalPacking::PackShadowSignal(signal);
}

#endif
