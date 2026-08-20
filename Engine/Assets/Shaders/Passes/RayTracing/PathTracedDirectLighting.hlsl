#include "Resources/FrameUniformData.hlsli"
#include "Resources/ViewCameraUniformData.hlsli"
#include "Resources/LightConstantBufferData.hlsli"

#include "Passes/Deferred/GBufferUtils.hlsli"
#include "Lighting/AreaLights.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "RayTracing/PathTracedLightingUniform.hlsli"
#include "RayTracing/Shadows/RayTracedShadowVisibility.hlsli"

RWTexture2D<float4> DirectDiffuse;
RWTexture2D<float4> DirectSpecular;
RWTexture2D<float4> DirectSubsurface;

float2 BuildLightSample(float3 positionWorld, uint lightIndex, uint dimensionTag, uint sampleIndex)
{
	return LightSampling::StableLightSample2D(positionWorld, lightIndex, dimensionTag, FrameIndex + sampleIndex * 4099u);
}

void AccumulatePathTracedDirectLight(GBufferData surface,
                                     float3 positionWorld,
                                     float3 viewDirWorld,
                                     LightSampling::DirectLightSample lightSample,
                                     bool castsShadow,
                                     inout float3 diffuse,
                                     inout float3 specular,
                                     inout float3 subsurface)
{
	if (!lightSample.Valid)
	{
		return;
	}
	const ShadowVisibilitySignal shadow =
	    RayTracedShadowVisibility::TraceDirectLightSample(positionWorld, surface.NormalWorld, lightSample, castsShadow);
	float3 lightDiffuse = 0.0f.xxx;
	float3 lightSpecular = 0.0f.xxx;
	float3 lightSubsurface = 0.0f.xxx;
	SurfaceLighting::AccumulateDirectLightSample(viewDirWorld,
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
	                                             lightDiffuse,
	                                             lightSpecular,
	                                             lightSubsurface);
	diffuse += lightDiffuse;
	specular += lightSpecular;
	subsurface += lightSubsurface;
}

void EvaluatePathTracedDirectLighting(GBufferData surface,
                                      float3 positionWorld,
                                      float3 viewDirWorld,
                                      uint sampleIndex,
                                      out float3 diffuse,
                                      out float3 specular,
                                      out float3 subsurface)
{
	diffuse = 0.0f.xxx;
	specular = 0.0f.xxx;
	subsurface = 0.0f.xxx;
	[loop] for (uint i = 0u; i < ViewLighting.DirectionalLightCount; ++i)
	{
		AccumulatePathTracedDirectLight(
		    surface,
		    positionWorld,
		    viewDirWorld,
		    AreaLights::SampleDirectionalLight(i, BuildLightSample(positionWorld, i, LightSampling::LightTypeDirectional, sampleIndex)),
		    DirectionalLights[i].CastShadow != 0u,
		    diffuse,
		    specular,
		    subsurface);
	}

	[loop] for (uint i = 0u; i < ViewLighting.PointLightCount; ++i)
	{
		AccumulatePathTracedDirectLight(
		    surface,
		    positionWorld,
		    viewDirWorld,
		    AreaLights::SamplePointLight(positionWorld, i, BuildLightSample(positionWorld, i, LightSampling::LightTypePoint, sampleIndex)),
		    PointLights[i].CastShadow != 0u,
		    diffuse,
		    specular,
		    subsurface);
	}

	[loop] for (uint i = 0u; i < ViewLighting.SpotLightCount; ++i)
	{
		AccumulatePathTracedDirectLight(
		    surface,
		    positionWorld,
		    viewDirWorld,
		    AreaLights::SampleSpotLight(positionWorld, i, BuildLightSample(positionWorld, i, LightSampling::LightTypeSpot, sampleIndex)),
		    SpotLights[i].CastShadow != 0u,
		    diffuse,
		    specular,
		    subsurface);
	}

	[loop] for (uint i = 0u; i < ViewLighting.RectLightCount; ++i)
	{
		AccumulatePathTracedDirectLight(
		    surface,
		    positionWorld,
		    viewDirWorld,
		    AreaLights::SampleRectLight(positionWorld, i, BuildLightSample(positionWorld, i, LightSampling::LightTypeRect, sampleIndex)),
		    RectLights[i].CastShadow != 0u,
		    diffuse,
		    specular,
		    subsurface);
	}
}

[numthreads(8, 8, 1)] void main(uint3 id : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	DirectDiffuse.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}
	const uint2 pixel = id.xy;
	const GBufferData g = LoadGBuffer(pixel);
	if (IsSkyPixel(g.SceneDepth))
	{
		DirectDiffuse[pixel] = 0.0f.xxxx;
		DirectSpecular[pixel] = 0.0f.xxxx;
		DirectSubsurface[pixel] = 0.0f.xxxx;
		return;
	}

	const float3 positionWorld = ReconstructGBufferWorldPosition(pixel, g.SceneDepth, InvViewMTX, InvProjectionMTX);
	const float3 viewDirWorld = normalize(Position - positionWorld);
	float3 diffuse = 0.0f.xxx;
	float3 specular = 0.0f.xxx;
	float3 subsurface = 0.0f.xxx;
	const uint sampleCount = max(PathTracedLightingSamplesPerPixel, 1u);

	[loop] for (uint sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
	{
		float3 sampleDiffuse;
		float3 sampleSpecular;
		float3 sampleSubsurface;
		EvaluatePathTracedDirectLighting(g, positionWorld, viewDirWorld, sampleIndex, sampleDiffuse, sampleSpecular, sampleSubsurface);
		diffuse += sampleDiffuse;
		specular += sampleSpecular;
		subsurface += sampleSubsurface;
	}

	const float invSampleCount = rcp(float(sampleCount));
	DirectDiffuse[pixel] = float4(diffuse * invSampleCount, 1.0f);
	DirectSpecular[pixel] = float4(specular * invSampleCount, 1.0f);
	DirectSubsurface[pixel] = float4(subsurface * invSampleCount, 1.0f);
}
