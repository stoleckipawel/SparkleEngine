#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> IndirectDiffuseTexture;
RWTexture2D<float4> IndirectDiffuseDemodulatedRadiance;
RWTexture2D<float4> IndirectDiffuseAlbedo;
RWTexture2D<float4> IndirectSpecularAlbedo;
RWTexture2D<float4> IndirectMaterialGuide;
RWTexture2D<float4> IndirectDiffuseSampleGuide;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

cbuffer IndirectDiffuseUniformData
{
	uint IndirectDiffuseDebugMode;
	uint RayTracingHitDataAvailable;
	float IndirectDiffuseNormalBias;
	float IndirectDiffuseMaxDistance;
	uint RayTracingHitInstanceCount;
	uint RayTracingHitMaterialCount;
	uint IndirectDiffuseMaterialTextureTableAvailable;
	uint IndirectDiffuseMaterialTextureTableDescriptorCount;
	uint IndirectDiffuseMaterialTextureTableCapacity;
	float IndirectDiffuseIntensity;
	uint IndirectDiffuseBounceCount;
	uint IndirectDiffusePadding1;
};

#include "RayTracing/PathLighting.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "Passes/Deferred/IndirectDiffuseDebug.hlsli"

static const uint IndirectDiffuseRayFlags = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint IndirectDiffuseInstanceMask = 0xFFu;
static const float IndirectDiffuseMinimumTMin = 0.001f;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	IndirectDiffuseTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	const GBufferData gBuffer = LoadGBuffer(pixelCoord);
	if (IsSkyPixel(gBuffer.DeviceZ))
	{
		IndirectDiffuseTexture[pixelCoord] = 0.0f.xxxx;
		IndirectDiffuseDemodulatedRadiance[pixelCoord] = 0.0f.xxxx;
		IndirectDiffuseAlbedo[pixelCoord] = 0.0f.xxxx;
		IndirectSpecularAlbedo[pixelCoord] = 0.0f.xxxx;
		IndirectMaterialGuide[pixelCoord] = 0.0f.xxxx;
		IndirectDiffuseSampleGuide[pixelCoord] = 0.0f.xxxx;
		return;
	}

	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(pixelCoord, gBuffer.DeviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const RayTracingPathSurface primarySurface =
	    BuildPrimaryRayTracingPathSurface(
	        positionWorld,
	        gBuffer.NormalWorld,
	        normalize(Camera.Position - positionWorld),
	        gBuffer.BaseColor,
	        gBuffer.Roughness,
	        gBuffer.Metallic,
	        gBuffer.DielectricF0);
	RayTracingPathTrace::TraceSettings traceSettings;
	traceSettings.NormalBias = IndirectDiffuseNormalBias;
	traceSettings.MaxDistance = IndirectDiffuseMaxDistance;
	traceSettings.MinT = IndirectDiffuseMinimumTMin;
	traceSettings.RayFlags = IndirectDiffuseRayFlags;
	traceSettings.InstanceMask = IndirectDiffuseInstanceMask;

	const RayTracingPathLighting::Result path =
	    RayTracingPathLighting::TraceSurfacePath(
	        SkyTexture,
	        SamplerLinearClamp,
	        primarySurface,
	        pixelCoord,
	        0u,
	        RayTracingPathSampling::SpecularSampleModeStochasticGGX,
	        IndirectDiffuseBounceCount,
	        traceSettings);

	RayTracingPathSample::LightingResult debugLighting = path.FirstLighting;
	debugLighting.Contribution =
	    path.PrimaryLobe == RayTracingPathSample::LobeDiffuse ? path.FinalContribution : 0.0f.xxx;

	const float3 finalContribution = debugLighting.Contribution * IndirectDiffuseIntensity;
	const float3 outputColor = IndirectDiffuseDebugMode == IndirectDiffuseDebug::Off
	                               ? finalContribution
	                               : IndirectDiffuseDebug::BuildColor(
	                                     IndirectDiffuseDebugMode,
	                                     path.FirstSample,
	                                     debugLighting,
	                                     IndirectDiffuseMaxDistance);
	const float diffuseSampleSelected = path.PrimaryLobe == RayTracingPathSample::LobeDiffuse ? 1.0f : 0.0f;
	const float diffuseSampleValid =
	    diffuseSampleSelected *
	    (path.FirstSample.RejectionReason == RayTracingPathSample::RejectionReasonNone ? 1.0f : 0.0f);
	const float diffuseHitValid = diffuseSampleSelected * (path.FirstLighting.Hit ? 1.0f : 0.0f);
	const float materialValid = 1.0f;
	const float3 diffuseAlbedo = gBuffer.BaseColor * (1.0f - gBuffer.Metallic);
	const float3 specularAlbedo = SurfaceLighting::BuildF0(gBuffer.BaseColor, gBuffer.Metallic, gBuffer.DielectricF0);

	IndirectDiffuseTexture[pixelCoord] = float4(outputColor, path.FirstLighting.Hit ? 1.0f : 0.0f);
	IndirectDiffuseDemodulatedRadiance[pixelCoord] =
	    float4(finalContribution / max(diffuseAlbedo, 1.0e-4f.xxx), diffuseSampleValid);
	IndirectDiffuseAlbedo[pixelCoord] = float4(diffuseAlbedo, materialValid);
	IndirectSpecularAlbedo[pixelCoord] = float4(specularAlbedo, materialValid);
	IndirectMaterialGuide[pixelCoord] = float4(gBuffer.Roughness, gBuffer.Metallic, gBuffer.DielectricF0, materialValid);
	IndirectDiffuseSampleGuide[pixelCoord] =
	    float4(
	        max(path.FirstLighting.HitDistance, 0.0f),
	        diffuseSampleValid,
	        float(RayTracingPathSample::LobeDiffuse),
	        diffuseHitValid);
}
