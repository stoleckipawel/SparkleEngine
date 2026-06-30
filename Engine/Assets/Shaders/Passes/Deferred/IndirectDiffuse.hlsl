#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> IndirectDiffuseTexture;
RaytracingAccelerationStructure SceneTlas;
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
	const int3 pixel = int3(pixelCoord, 0);
	const float deviceZ = LoadGBufferDeviceZ(pixelCoord);
	if (IsSkyPixel(deviceZ))
	{
		IndirectDiffuseTexture[pixelCoord] = 0.0f.xxxx;
		return;
	}

	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(pixel).xyz);
	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(pixelCoord, deviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float4 materialSample = GBufferMaterial.Load(pixel);
	const RayTracingPathSurface primarySurface =
	    BuildPrimaryRayTracingPathSurface(
	        positionWorld,
	        normalWorld,
	        normalize(Camera.Position - positionWorld),
	        saturate(GBufferBaseColor.Load(pixel).rgb),
	        saturate(materialSample.g),
	        saturate(materialSample.r),
	        DecodeGBufferDielectricF0(materialSample.a));
	RayTracingPathLighting::TraceSettings traceSettings;
	traceSettings.NormalBias = IndirectDiffuseNormalBias;
	traceSettings.MaxDistance = IndirectDiffuseMaxDistance;
	traceSettings.MinT = IndirectDiffuseMinimumTMin;
	traceSettings.RayFlags = IndirectDiffuseRayFlags;
	traceSettings.InstanceMask = IndirectDiffuseInstanceMask;

	const RayTracingPathLighting::Result path =
	    RayTracingPathLighting::TraceSurfacePath(
	        SceneTlas,
	        SkyTexture,
	        SamplerLinearClamp,
	        primarySurface,
	        pixelCoord,
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

	IndirectDiffuseTexture[pixelCoord] = float4(outputColor, path.FirstLighting.Hit ? 1.0f : 0.0f);
}
