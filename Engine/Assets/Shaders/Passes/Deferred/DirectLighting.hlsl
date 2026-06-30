#include "Lighting/AreaLights.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSampling.hlsli"
#if defined(SPARKLE_DIRECT_LIGHTING_NO_RAY_QUERY)
#include "RayTracing/Shadows/RayTracedShadowSignals.hlsli"
#else
#include "RayTracing/Shadows/RayTracedShadowTrace.hlsli"
#endif
RWTexture2D<float4> DirectDiffuseTexture;
RWTexture2D<float4> DirectSpecularTexture;
RWTexture2D<float4> DirectSubsurfaceTexture;

ShadowVisibilitySignal TraceLightSampleVisibility(
    float3 positionWorld,
    float3 normalWorld,
    LightSampling::DirectLightSample lightSample,
    bool castsShadow)
{
#if defined(SPARKLE_DIRECT_LIGHTING_NO_RAY_QUERY)
	return RayTracedShadowSignals::BuildUnshadowedSignal(lightSample.VisibilityDistance);
#else
	return RayTracedShadows::TraceDirectLightSample(
	    positionWorld,
	    normalWorld,
	    lightSample.DirectionWorld,
	    lightSample.VisibilityDistance,
	    lightSample.IsDirectional,
	    castsShadow);
#endif
}

void AddDirectLightSample(
    GBufferData gBuffer,
    float3 positionWorld,
    float3 viewDirWorld,
    bool evaluateSubsurface,
    LightSampling::DirectLightSample lightSample,
    bool castsShadow,
    inout float3 directDiffuse,
    inout float3 directSpecular,
    inout float3 directSubsurface)
{
	if (!lightSample.Valid)
	{
		return;
	}

	const ShadowVisibilitySignal shadow = TraceLightSampleVisibility(positionWorld, gBuffer.NormalWorld, lightSample, castsShadow);
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
	const bool evaluateSubsurface = any(gBuffer.SubsurfaceColor > 0.0f.xxx) && gBuffer.SubsurfaceStrength > 0.0f;
	const uint directionalLightCount = ViewLighting.DirectionalLightCount;
	const uint pointLightCount = ViewLighting.PointLightCount;
	const uint spotLightCount = ViewLighting.SpotLightCount;
	const uint rectLightCount = ViewLighting.RectLightCount;

	[loop] for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		AddDirectLightSample(
		    gBuffer,
		    positionWorld,
		    viewDirWorld,
		    evaluateSubsurface,
		    AreaLights::SampleDirectionalLight(
		        lightIndex,
		        RayTracedShadowSampling::BuildAnimatedSample(dispatchThreadId.xy, lightIndex, 0u)),
		    DirectionalLights[lightIndex].CastShadow != 0u,
		    directDiffuse,
		    directSpecular,
		    directSubsurface);
	}

	[loop] for (uint lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
	{
		AddDirectLightSample(
		    gBuffer,
		    positionWorld,
		    viewDirWorld,
		    evaluateSubsurface,
		    AreaLights::SamplePointLight(
		        positionWorld,
		        lightIndex,
		        RayTracedShadowSampling::BuildAnimatedSample(dispatchThreadId.xy, lightIndex, 1u)),
		    PointLights[lightIndex].CastShadow != 0u,
		    directDiffuse,
		    directSpecular,
		    directSubsurface);
	}

	[loop] for (uint lightIndex = 0; lightIndex < spotLightCount; ++lightIndex)
	{
		AddDirectLightSample(
		    gBuffer,
		    positionWorld,
		    viewDirWorld,
		    evaluateSubsurface,
		    AreaLights::SampleSpotLight(
		        positionWorld,
		        lightIndex,
		        RayTracedShadowSampling::BuildAnimatedSample(dispatchThreadId.xy, lightIndex, 2u)),
		    SpotLights[lightIndex].CastShadow != 0u,
		    directDiffuse,
		    directSpecular,
		    directSubsurface);
	}

	[loop] for (uint lightIndex = 0; lightIndex < rectLightCount; ++lightIndex)
	{
		AddDirectLightSample(
		    gBuffer,
		    positionWorld,
		    viewDirWorld,
		    evaluateSubsurface,
		    AreaLights::SampleRectLight(
		        positionWorld,
		        lightIndex,
		        RayTracedShadowSampling::BuildAnimatedSample(dispatchThreadId.xy, lightIndex, 3u)),
		    RectLights[lightIndex].CastShadow != 0u,
		    directDiffuse,
		    directSpecular,
		    directSubsurface);
	}

	DirectDiffuseTexture[dispatchThreadId.xy] = float4(directDiffuse, gBuffer.Alpha);
	DirectSpecularTexture[dispatchThreadId.xy] = float4(directSpecular, gBuffer.Alpha);
	DirectSubsurfaceTexture[dispatchThreadId.xy] = float4(directSubsurface, gBuffer.Alpha);
}
