#include "Lighting/DirectLightSampling.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSampling.hlsli"
#include "RayTracing/Shadows/RayTracedShadowVisibility.hlsli"

RWTexture2D<float4> ShadowVisibilitySignalTexture;
RWTexture2D<float4> ShadowLightSampleTexture;

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
		ShadowLightSampleTexture[dispatchThreadId.xy] = DirectLightSampling::PackLightCandidate(DirectLightSampling::InvalidLightCandidate());
		return;
	}

	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, deviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(int3(dispatchThreadId.xy, 0)).xyz);
	const float candidateRandom =
	    RayTracedShadowSampling::BuildAnimatedSample(dispatchThreadId.xy, 0u, 0xA53u).x;
	const DirectLightSampling::LightCandidate candidate =
	    DirectLightSampling::SampleLightCandidate(positionWorld, normalWorld, candidateRandom);
	ShadowLightSampleTexture[dispatchThreadId.xy] = DirectLightSampling::PackLightCandidate(candidate);

	if (!DirectLightSampling::IsValid(candidate))
	{
		ShadowVisibilitySignalTexture[dispatchThreadId.xy] =
		    RayTracedShadowDenoiserInputs::PackShadowSignal(RayTracedShadowSignals::BuildUnshadowedSignal(0.0f));
		return;
	}

	const float2 shapeSample =
	    RayTracedShadowSampling::BuildAnimatedSample(dispatchThreadId.xy, candidate.Light.Index, candidate.Light.Type);
	const LightSampling::DirectLightSample lightSample =
	    DirectLightSampling::SampleDirectLight(candidate, positionWorld, shapeSample);
	const ShadowVisibilitySignal shadowSignal =
	    RayTracedShadowVisibility::TraceDirectLightSample(positionWorld, normalWorld, lightSample, DirectLightSampling::CastsShadow(candidate.Light));

	ShadowVisibilitySignalTexture[dispatchThreadId.xy] = RayTracedShadowDenoiserInputs::PackShadowSignal(shadowSignal);
}
