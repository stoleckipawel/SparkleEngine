#include "Resources/ConstantBuffers.hlsli"
#include "Lighting/AreaLights.hlsli"
#include "Lighting/SkyEnvironment.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "RayTracing/Shadows/RayTracedShadowVisibility.hlsli"
#include "RayTracing/PathLighting.hlsli"

RWTexture2D<float4> ReferenceSceneColorTexture;
RWTexture2D<float4> ReferenceDirectTexture;
RWTexture2D<float4> ReferenceIndirectDiffuseTexture;
RWTexture2D<float4> ReferenceIndirectSpecularTexture;
RWTexture2D<float> ReferencePrimaryDeviceDepthTexture;
RWTexture2D<float4> ReferencePrimaryNormalTexture;
RWTexture2D<float4> ReferencePrimaryDiffuseAlbedoTexture;
RWTexture2D<float4> ReferencePrimarySpecularAlbedoTexture;
RWTexture2D<float4> ReferencePrimaryMaterialGuideTexture;
RWTexture2D<float4> ReferencePrimaryPathSampleGuideTexture;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

cbuffer ReferencePathTracingUniformData
{
	uint ReferencePathTracingSamplesPerPixel;
	uint ReferencePathTracingBounceCount;
	float ReferencePathTracingNormalBias;
	float ReferencePathTracingMaxDistance;
};

static const uint ReferencePathTracingRayFlags = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint ReferencePathTracingInstanceMask = 0xFFu;
static const float ReferencePathTracingMinimumTMin = 0.001f;

float ComputeReferencePrimaryDeviceDepth(float3 positionWorld)
{
	const float4 clipPosition = mul(float4(positionWorld, 1.0f), Camera.ViewProjMTX);
	return abs(clipPosition.w) > 1.0e-6f ? saturate(clipPosition.z / clipPosition.w) : 0.0f;
}

void ClearReferenceGuides(uint2 pixelCoord)
{
	ReferencePrimaryDeviceDepthTexture[pixelCoord] = 0.0f;
	ReferencePrimaryNormalTexture[pixelCoord] = 0.0f.xxxx;
	ReferencePrimaryDiffuseAlbedoTexture[pixelCoord] = 0.0f.xxxx;
	ReferencePrimarySpecularAlbedoTexture[pixelCoord] = 0.0f.xxxx;
	ReferencePrimaryMaterialGuideTexture[pixelCoord] = 0.0f.xxxx;
	ReferencePrimaryPathSampleGuideTexture[pixelCoord] = 0.0f.xxxx;
}

float2 BuildReferenceLightSample(float3 positionWorld, uint lightIndex, uint dimensionTag, uint sampleIndex)
{
	return LightSampling::StableLightSample2D(
	    positionWorld,
	    lightIndex,
	    dimensionTag,
	    FrameIndex + sampleIndex * 4099u);
}

void AccumulateReferenceDirectLightSample(
    RayTracingHitSurfaceData surface,
    float3 viewDirWorld,
    LightSampling::DirectLightSample lightSample,
    bool castsShadow,
    inout float3 directRadiance)
{
	if (!lightSample.Valid)
	{
		return;
	}

	const ShadowVisibilitySignal shadow =
	    RayTracedShadowVisibility::TraceDirectLightSample(
	        surface.PositionWorld,
	        surface.NormalWorld,
	        lightSample,
	        castsShadow);
	float3 diffuse = 0.0f.xxx;
	float3 specular = 0.0f.xxx;
	float3 subsurface = 0.0f.xxx;
	SurfaceLighting::AccumulateDirectLightSample(
	    viewDirWorld,
	    surface.NormalWorld,
	    surface.BaseColor,
	    surface.Roughness,
	    surface.Metallic,
	    surface.DielectricF0,
	    surface.SubsurfaceColor,
	    surface.SubsurfaceStrength,
	    any(surface.SubsurfaceColor > 0.0f.xxx) && surface.SubsurfaceStrength > 0.0f,
	    lightSample,
	    shadow.Visibility,
	    diffuse,
	    specular,
	    subsurface);

	directRadiance += diffuse + specular + subsurface;
}

float3 EvaluateReferenceDirectLighting(
    RayTracingHitSurfaceData surface,
    float3 rayDirectionWorld,
    uint sampleIndex)
{
	float3 directRadiance = 0.0f.xxx;
	const float3 viewDirWorld = normalize(-rayDirectionWorld);

	[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.DirectionalLightCount; ++lightIndex)
	{
		AccumulateReferenceDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SampleDirectionalLight(
		        lightIndex,
		        BuildReferenceLightSample(surface.PositionWorld, lightIndex, LightSampling::LightTypeDirectional, sampleIndex)),
		    DirectionalLights[lightIndex].CastShadow != 0u,
		    directRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.PointLightCount; ++lightIndex)
	{
		AccumulateReferenceDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SamplePointLight(
		        surface.PositionWorld,
		        lightIndex,
		        BuildReferenceLightSample(surface.PositionWorld, lightIndex, LightSampling::LightTypePoint, sampleIndex)),
		    PointLights[lightIndex].CastShadow != 0u,
		    directRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.SpotLightCount; ++lightIndex)
	{
		AccumulateReferenceDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SampleSpotLight(
		        surface.PositionWorld,
		        lightIndex,
		        BuildReferenceLightSample(surface.PositionWorld, lightIndex, LightSampling::LightTypeSpot, sampleIndex)),
		    SpotLights[lightIndex].CastShadow != 0u,
		    directRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.RectLightCount; ++lightIndex)
	{
		AccumulateReferenceDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SampleRectLight(
		        surface.PositionWorld,
		        lightIndex,
		        BuildReferenceLightSample(surface.PositionWorld, lightIndex, LightSampling::LightTypeRect, sampleIndex)),
		    RectLights[lightIndex].CastShadow != 0u,
		    directRadiance);
	}

	return directRadiance;
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	ReferenceSceneColorTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	const float3 primaryRayOrigin = Camera.Position;
	const float3 primaryRayDirection = ComputeSkyViewDirectionWorld(pixelCoord);
	RayTracingPathTrace::TraceSettings traceSettings;
	traceSettings.NormalBias = ReferencePathTracingNormalBias;
	traceSettings.MaxDistance = ReferencePathTracingMaxDistance;
	traceSettings.MinT = ReferencePathTracingMinimumTMin;
	traceSettings.RayFlags = ReferencePathTracingRayFlags;
	traceSettings.InstanceMask = ReferencePathTracingInstanceMask;

	const RayTracingTraceResult primaryTrace =
	    RayTracingPathTrace::TraceSceneRay(primaryRayOrigin, primaryRayDirection, traceSettings);
	if (!primaryTrace.Hit)
	{
		const float3 skyRadiance =
		    SampleSkyEnvironmentRadiance(SkyTexture, SamplerLinearClamp, primaryRayDirection);
		ReferenceSceneColorTexture[pixelCoord] = float4(skyRadiance, 1.0f);
		ReferenceDirectTexture[pixelCoord] = 0.0f.xxxx;
		ReferenceIndirectDiffuseTexture[pixelCoord] = 0.0f.xxxx;
		ReferenceIndirectSpecularTexture[pixelCoord] = 0.0f.xxxx;
		ClearReferenceGuides(pixelCoord);
		return;
	}

	const RayTracingHitSurfaceData primaryHit =
	    ReconstructRayTracingHitSurface(primaryTrace, primaryRayOrigin, primaryRayDirection);
	if (!primaryHit.Valid)
	{
		ReferenceSceneColorTexture[pixelCoord] = 0.0f.xxxx;
		ReferenceDirectTexture[pixelCoord] = 0.0f.xxxx;
		ReferenceIndirectDiffuseTexture[pixelCoord] = 0.0f.xxxx;
		ReferenceIndirectSpecularTexture[pixelCoord] = 0.0f.xxxx;
		ClearReferenceGuides(pixelCoord);
		return;
	}

	const RayTracingPathSurface primarySurface =
	    BuildHitRayTracingPathSurface(primaryHit, primaryRayDirection);
	const float3 primaryDiffuseAlbedo = primaryHit.BaseColor * (1.0f - primaryHit.Metallic);
	const float3 primarySpecularAlbedo =
	    SurfaceLighting::BuildF0(primaryHit.BaseColor, primaryHit.Metallic, primaryHit.DielectricF0);

	float3 directRadiance = 0.0f.xxx;
	float3 indirectDiffuseRadiance = 0.0f.xxx;
	float3 indirectSpecularRadiance = 0.0f.xxx;
	float4 primaryPathSampleGuide = 0.0f.xxxx;
	const uint sampleCount = max(ReferencePathTracingSamplesPerPixel, 1u);

	[loop] for (uint sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
	{
		directRadiance += EvaluateReferenceDirectLighting(primaryHit, primaryRayDirection, sampleIndex);

		const RayTracingPathLighting::Result path =
		    RayTracingPathLighting::TraceSurfacePath(
		        SkyTexture,
		        SamplerLinearClamp,
		        primarySurface,
		        pixelCoord,
		        sampleIndex,
		        RayTracingPathSampling::SpecularSampleModeStochasticGGX,
		        ReferencePathTracingBounceCount,
		        traceSettings);
		if (path.PrimaryLobe == RayTracingPathSample::LobeDiffuse)
		{
			indirectDiffuseRadiance += path.FinalContribution;
		}
		else if (path.PrimaryLobe == RayTracingPathSample::LobeSpecular)
		{
			indirectSpecularRadiance += path.FinalContribution;
		}
		if (sampleIndex == 0u)
		{
			const float pathSampleValid =
			    path.FirstSample.RejectionReason == RayTracingPathSample::RejectionReasonNone ? 1.0f : 0.0f;
			primaryPathSampleGuide =
			    float4(max(path.FirstLighting.HitDistance, 0.0f), pathSampleValid, float(path.PrimaryLobe), path.FirstLighting.Hit ? 1.0f : 0.0f);
		}
	}

	const float invSampleCount = rcp(float(sampleCount));
	directRadiance *= invSampleCount;
	indirectDiffuseRadiance *= invSampleCount;
	indirectSpecularRadiance *= invSampleCount;

	ReferenceDirectTexture[pixelCoord] = float4(directRadiance, primaryHit.Alpha);
	ReferenceIndirectDiffuseTexture[pixelCoord] = float4(indirectDiffuseRadiance, primaryHit.Alpha);
	ReferenceIndirectSpecularTexture[pixelCoord] = float4(indirectSpecularRadiance, primaryHit.Alpha);
	ReferencePrimaryDeviceDepthTexture[pixelCoord] = ComputeReferencePrimaryDeviceDepth(primaryHit.PositionWorld);
	ReferencePrimaryNormalTexture[pixelCoord] = float4(primaryHit.NormalWorld, primaryHit.Alpha);
	ReferencePrimaryDiffuseAlbedoTexture[pixelCoord] = float4(primaryDiffuseAlbedo, 1.0f);
	ReferencePrimarySpecularAlbedoTexture[pixelCoord] = float4(primarySpecularAlbedo, 1.0f);
	ReferencePrimaryMaterialGuideTexture[pixelCoord] =
	    float4(primaryHit.Roughness, primaryHit.Metallic, primaryHit.DielectricF0, 1.0f);
	ReferencePrimaryPathSampleGuideTexture[pixelCoord] = primaryPathSampleGuide;
	ReferenceSceneColorTexture[pixelCoord] =
	    float4(directRadiance + indirectDiffuseRadiance + indirectSpecularRadiance + primaryHit.EmissiveColor, primaryHit.Alpha);
}
