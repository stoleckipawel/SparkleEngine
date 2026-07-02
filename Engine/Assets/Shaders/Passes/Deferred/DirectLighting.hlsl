#include "Lighting/DirectLightSampling.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSampling.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSignalPacking.hlsli"
RWTexture2D<float4> DirectDiffuseTexture;
RWTexture2D<float4> DirectSpecularTexture;
RWTexture2D<float4> DirectSubsurfaceTexture;
Texture2D<float4> ShadowVisibilitySignalTexture;
Texture2D<float4> ShadowLightSampleTexture;

void AddDirectLightSample(
    GBufferData gBuffer,
    float3 viewDirWorld,
    bool evaluateSubsurface,
    LightSampling::DirectLightSample lightSample,
    ShadowVisibilitySignal shadow,
    inout float3 directDiffuse,
    inout float3 directSpecular,
    inout float3 directSubsurface)
{
	if (!lightSample.Valid)
	{
		return;
	}

	float3 lightDiffuse;
	float3 lightSpecular;
	float3 lightSubsurface;
	SurfaceLighting::AccumulateDirectLightSample(
	    viewDirWorld,
	    gBuffer.NormalWorld,
	    gBuffer.BaseColor,
	    gBuffer.Roughness,
	    gBuffer.Metallic,
	    gBuffer.DielectricF0,
	    gBuffer.SubsurfaceColor,
	    gBuffer.SubsurfaceStrength,
	    evaluateSubsurface,
	    lightSample,
	    shadow.Visibility,
	    lightDiffuse,
	    lightSpecular,
	    lightSubsurface);

	directDiffuse += lightDiffuse;
	directSpecular += lightSpecular;
	directSubsurface += lightSubsurface;
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	DirectDiffuseTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const GBufferData gBuffer = LoadGBuffer(dispatchThreadId.xy);
	if (IsSkyPixel(gBuffer.DeviceZ))
	{
		DirectDiffuseTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		DirectSpecularTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		DirectSubsurfaceTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		return;
	}

	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, gBuffer.DeviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 viewDirWorld = normalize(Camera.Position - positionWorld);

	float3 directDiffuse = 0.0f;
	float3 directSpecular = 0.0f;
	float3 directSubsurface = 0.0f;
	const DirectLightSampling::LightCandidate lightCandidate =
	    DirectLightSampling::UnpackLightCandidate(ShadowLightSampleTexture.Load(int3(dispatchThreadId.xy, 0)));
	const ShadowVisibilitySignal shadowSignal =
	    RayTracedShadowSignalPacking::UnpackShadowSignal(ShadowVisibilitySignalTexture.Load(int3(dispatchThreadId.xy, 0)));
	const bool evaluateSubsurface = any(gBuffer.SubsurfaceColor > 0.0f.xxx) && gBuffer.SubsurfaceStrength > 0.0f;
	if (DirectLightSampling::IsValid(lightCandidate))
	{
		const float2 shapeSample =
		    RayTracedShadowSampling::BuildAnimatedSample(dispatchThreadId.xy, lightCandidate.Light.Index, lightCandidate.Light.Type);
		const LightSampling::DirectLightSample lightSample =
		    DirectLightSampling::SampleDirectLight(lightCandidate, positionWorld, shapeSample);

		AddDirectLightSample(
		    gBuffer,
		    viewDirWorld,
		    evaluateSubsurface,
		    lightSample,
		    shadowSignal,
		    directDiffuse,
		    directSpecular,
		    directSubsurface);
	}

	DirectDiffuseTexture[dispatchThreadId.xy] = float4(directDiffuse, gBuffer.Alpha);
	DirectSpecularTexture[dispatchThreadId.xy] = float4(directSpecular, gBuffer.Alpha);
	DirectSubsurfaceTexture[dispatchThreadId.xy] = float4(directSubsurface, gBuffer.Alpha);
}
