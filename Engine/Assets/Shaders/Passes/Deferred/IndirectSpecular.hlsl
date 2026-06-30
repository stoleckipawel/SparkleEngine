#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/RayTracingDebugModes.hlsli"

RWTexture2D<float4> IndirectSpecularTexture;
RaytracingAccelerationStructure SceneTlas;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

cbuffer IndirectSpecularUniformData
{
	uint IndirectSpecularDebugMode;
	uint RayTracingHitDataAvailable;
	float IndirectSpecularNormalBias;
	float IndirectSpecularMaxDistance;
	uint RayTracingHitInstanceCount;
	uint RayTracingHitMaterialCount;
	uint IndirectSpecularSampleMode;
	uint IndirectSpecularMaterialTextureTableAvailable;
	uint IndirectSpecularMaterialTextureTableDescriptorCount;
	uint IndirectSpecularMaterialTextureTableCapacity;
	uint IndirectSpecularBounceCount;
	uint IndirectSpecularPadding1;
};

#include "RayTracing/PathLighting.hlsli"

static const uint IndirectSpecularRayFlags = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint IndirectSpecularInstanceMask = 0xFFu;
static const uint IndirectSpecularDebugMirrorDirection = 3u;
static const uint IndirectSpecularDebugSampleDirection = 10u;
static const uint IndirectSpecularDebugSamplePdf = 11u;
static const uint IndirectSpecularDebugSampleThroughput = 12u;
static const uint IndirectSpecularDebugHitRadiance = 13u;
static const uint IndirectSpecularDebugFinalContribution = 14u;
static const float IndirectSpecularMinimumTMin = 0.001f;

struct IndirectSpecularResolvedContribution
{
	float3 HitRadiance;
	float3 FinalContribution;
};

#include "Passes/Deferred/IndirectSpecularDebug.hlsli"

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	IndirectSpecularTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	const int3 pixel = int3(pixelCoord, 0);
	const float deviceZ = LoadGBufferDeviceZ(pixelCoord);
	if (IsSkyPixel(deviceZ))
	{
		IndirectSpecularTexture[pixelCoord] = 0.0f.xxxx;
		return;
	}

	const float3 baseColor = saturate(GBufferBaseColor.Load(pixel).rgb);
	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(pixel).xyz);
	const float4 materialSample = GBufferMaterial.Load(pixel);
	const float roughness = saturate(materialSample.g);
	const float metallic = saturate(materialSample.r);
	const float dielectricF0 = DecodeGBufferDielectricF0(materialSample.a);
	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(pixelCoord, deviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 viewDirWorld = normalize(Camera.Position - positionWorld);
	RayTracingPathLighting::TraceSettings traceSettings;
	traceSettings.NormalBias = IndirectSpecularNormalBias;
	traceSettings.MaxDistance = IndirectSpecularMaxDistance;
	traceSettings.MinT = IndirectSpecularMinimumTMin;
	traceSettings.RayFlags = IndirectSpecularRayFlags;
	traceSettings.InstanceMask = IndirectSpecularInstanceMask;

	const RayTracingPathLighting::Result path =
	    RayTracingPathLighting::TraceSurfacePath(
	        SceneTlas,
	        SkyTexture,
	        SamplerLinearClamp,
	        BuildPrimaryRayTracingPathSurface(
	            positionWorld,
	            normalWorld,
	            viewDirWorld,
	            baseColor,
	            roughness,
	            metallic,
	            dielectricF0),
	        pixelCoord,
	        IndirectSpecularSampleMode,
	        IndirectSpecularBounceCount,
	        traceSettings);

	IndirectSpecularResolvedContribution resolved;
	resolved.HitRadiance = path.FirstLighting.IncidentRadiance;
	resolved.FinalContribution =
	    path.PrimaryLobe == RayTracingPathSample::LobeSpecular ? path.FinalContribution : 0.0f.xxx;

	const float3 mirrorDirectionWorld = normalize(reflect(-viewDirWorld, normalWorld));
	const float3 debugColor =
	    BuildIndirectSpecularDebugColor(
	        path.FirstTrace,
	        path.FirstHitSurface,
	        path.FirstSample,
	        resolved,
	        mirrorDirectionWorld) *
	    lerp(0.65f.xxx, baseColor, 0.35f);
	const float3 reflectionColor = IndirectSpecularDebugMode == RayTracingDebugModes::Off ? resolved.FinalContribution : debugColor;
	const float bindingKeepAliveSignal = float(FrameIndex & 1u) * 1.0e-6f + roughness * 1.0e-9f;

	IndirectSpecularTexture[pixelCoord] = float4(reflectionColor, path.FirstHitSurface.Valid ? 1.0f : bindingKeepAliveSignal);
}
