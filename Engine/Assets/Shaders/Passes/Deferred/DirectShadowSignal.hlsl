#include "Lighting/AreaLights.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/Shadows/DirectShadowSelection.hlsli"
#include "RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSampling.hlsli"
#include "RayTracing/Shadows/RayTracedShadowVisibility.hlsli"

RWTexture2D<float4> ShadowVisibilitySignalTexture;

LightSampling::DirectLightSample SampleSelectedLight(
    DirectShadowSelection::SelectedLight selectedLight,
    float3 positionWorld,
    uint2 pixel)
{
	if (selectedLight.Type == LightSampling::LightTypeInvalid)
	{
		return LightSampling::InvalidDirectLightSample();
	}

	if (selectedLight.Type == LightSampling::LightTypeDirectional)
	{
		return AreaLights::SampleDirectionalLight(
		    selectedLight.Index,
		    RayTracedShadowSampling::BuildAnimatedSample(pixel, selectedLight.Index, LightSampling::LightTypeDirectional));
	}

	if (selectedLight.Type == LightSampling::LightTypePoint)
	{
		return AreaLights::SamplePointLight(
		    positionWorld,
		    selectedLight.Index,
		    RayTracedShadowSampling::BuildAnimatedSample(pixel, selectedLight.Index, LightSampling::LightTypePoint));
	}

	if (selectedLight.Type == LightSampling::LightTypeSpot)
	{
		return AreaLights::SampleSpotLight(
		    positionWorld,
		    selectedLight.Index,
		    RayTracedShadowSampling::BuildAnimatedSample(pixel, selectedLight.Index, LightSampling::LightTypeSpot));
	}

	if (selectedLight.Type == LightSampling::LightTypeRect)
	{
		return AreaLights::SampleRectLight(
		    positionWorld,
		    selectedLight.Index,
		    RayTracedShadowSampling::BuildAnimatedSample(pixel, selectedLight.Index, LightSampling::LightTypeRect));
	}

	return LightSampling::InvalidDirectLightSample();
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	ShadowVisibilitySignalTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float deviceZ = LoadGBufferDeviceZ(dispatchThreadId.xy);
	if (IsSkyPixel(deviceZ))
	{
		ShadowVisibilitySignalTexture[dispatchThreadId.xy] =
		    RayTracedShadowDenoiserInputs::PackShadowSignal(RayTracedShadowSignals::BuildUnshadowedSignal(0.0f));
		return;
	}

	const DirectShadowSelection::SelectedLight selectedLight = DirectShadowSelection::SelectFirstShadowCastingLight();
	if (selectedLight.Type == LightSampling::LightTypeInvalid)
	{
		ShadowVisibilitySignalTexture[dispatchThreadId.xy] =
		    RayTracedShadowDenoiserInputs::PackShadowSignal(RayTracedShadowSignals::BuildUnshadowedSignal(0.0f));
		return;
	}

	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, deviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(int3(dispatchThreadId.xy, 0)).xyz);
	const LightSampling::DirectLightSample lightSample = SampleSelectedLight(selectedLight, positionWorld, dispatchThreadId.xy);
	const ShadowVisibilitySignal shadowSignal =
	    RayTracedShadowVisibility::TraceDirectLightSample(positionWorld, normalWorld, lightSample, true);

	ShadowVisibilitySignalTexture[dispatchThreadId.xy] = RayTracedShadowDenoiserInputs::PackShadowSignal(shadowSignal);
}
