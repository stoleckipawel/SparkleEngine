#include "Resources/ConstantBuffers.hlsli"
#include "Common/Math.hlsli"
#include "Common/Random.hlsli"
#include "BRDF/SpecularSampling.hlsli"
#include "Lighting/SkyEnvironment.hlsli"
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

#include "RayTracing/PathSurface.hlsli"
#include "RayTracing/RayTracingTraceQuery.hlsli"
#include "RayTracing/RayTracingHitLighting.hlsli"

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

float2 GenerateReflectionRandomSample(uint2 pixelCoord, uint bounceIndex)
{
	return CommonRandom::InterleavedGradientNoise2(
	    float2(pixelCoord) + float2(bounceIndex * 23u, bounceIndex * 31u),
	    FrameIndex + bounceIndex * 149u,
	    float2(41.0f, 137.0f));
}

float3 EvaluateIndirectSpecularSampleThroughput(
    RayTracingPathSurface surface,
    BRDF::SpecularSampling::LobeSample sample)
{
	if (!surface.Valid || !sample.Valid)
	{
		return 0.0f.xxx;
	}

	const BRDF::ShadingData shadingData =
	    BRDF::ComputeShadingData(surface.NormalWorld, surface.ViewDirWorld, sample.DirectionWorld);
	if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f)
	{
		return 0.0f.xxx;
	}

	const float3 f0 = SurfaceLighting::BuildF0(surface.BaseColor, surface.Metallic, surface.DielectricF0);
	const float3 fresnel = BRDF::Fresnel::EvaluateDirect(shadingData.VoH, f0);
	if (sample.Mirror)
	{
		return fresnel;
	}

	const float3 specularBrdf = BRDF::Specular::EvaluateDirect(shadingData, surface.Roughness, fresnel);
	return max(specularBrdf * (shadingData.NoL * rcp(max(sample.Pdf, 1.0e-4f))), 0.0f.xxx);
}

float3 ComputeRayOrigin(float3 positionWorld, float3 normalWorld, float3 rayDirectionWorld)
{
	const float bias = max(IndirectSpecularNormalBias, 0.0f);
	const float NoR = abs(dot(normalWorld, rayDirectionWorld));
	const float grazingScale = rcp(max(NoR, 0.25f));
	return positionWorld + normalWorld * bias * grazingScale + rayDirectionWorld * IndirectSpecularMinimumTMin;
}

RayTracingTraceResult TraceIndirectSpecularRay(float3 positionWorld, float3 normalWorld, float3 reflectionDirectionWorld)
{
	const float3 rayDirectionWorld = SafeNormalize(reflectionDirectionWorld, normalWorld);
	return TraceRayQueryWithAlphaTest(
	    SceneTlas,
	    ComputeRayOrigin(positionWorld, normalWorld, rayDirectionWorld),
	    rayDirectionWorld,
	    IndirectSpecularMinimumTMin,
	    max(IndirectSpecularMaxDistance, IndirectSpecularMinimumTMin),
	    IndirectSpecularRayFlags,
	    IndirectSpecularInstanceMask);
}

IndirectSpecularResolvedContribution ResolveIndirectSpecularPath(
    uint2 pixelCoord,
    float3 primaryPositionWorld,
    float3 primaryNormalWorld,
    float3 primaryViewDirWorld,
    float3 primaryBaseColor,
    float primaryRoughness,
    float primaryMetallic,
    float primaryDielectricF0,
    out BRDF::SpecularSampling::LobeSample firstSample,
    out RayTracingTraceResult firstTrace,
    out RayTracingHitSurfaceData firstHitSurface)
{
	RayTracingPathSurface pathSurface =
	    BuildPrimaryRayTracingPathSurface(
	        primaryPositionWorld,
	        primaryNormalWorld,
	        primaryViewDirWorld,
	        primaryBaseColor,
	        primaryRoughness,
	        primaryMetallic,
	        primaryDielectricF0);
	firstSample =
	    BRDF::SpecularSampling::SampleReflectionLobe(
	        pathSurface.NormalWorld,
	        pathSurface.ViewDirWorld,
	        pathSurface.Roughness,
	        IndirectSpecularSampleMode,
	        GenerateReflectionRandomSample(pixelCoord, 0u));
	firstSample.Throughput = EvaluateIndirectSpecularSampleThroughput(pathSurface, firstSample);
	firstTrace = (RayTracingTraceResult) 0;
	firstHitSurface = (RayTracingHitSurfaceData) 0;

	float3 pathThroughput = 1.0f.xxx;
	float3 pathContribution = 0.0f.xxx;
	float3 firstHitRadiance = 0.0f.xxx;
	const uint bounceCount = max(IndirectSpecularBounceCount, 1u);

	[loop] for (uint bounceIndex = 0u; bounceIndex < bounceCount; ++bounceIndex)
	{
		BRDF::SpecularSampling::LobeSample sample =
		    BRDF::SpecularSampling::SampleReflectionLobe(
		        pathSurface.NormalWorld,
		        pathSurface.ViewDirWorld,
		        pathSurface.Roughness,
		        IndirectSpecularSampleMode,
		        GenerateReflectionRandomSample(pixelCoord, bounceIndex));
		sample.Throughput = EvaluateIndirectSpecularSampleThroughput(pathSurface, sample);
		pathThroughput *= sample.Throughput;
		if (max(max(pathThroughput.r, pathThroughput.g), pathThroughput.b) <= 1.0e-4f)
		{
			break;
		}

		const RayTracingTraceResult trace =
		    TraceIndirectSpecularRay(pathSurface.PositionWorld, pathSurface.NormalWorld, sample.DirectionWorld);
		const RayTracingHitSurfaceData hitSurface =
		    ReconstructRayTracingHitSurface(
		        trace,
		        ComputeRayOrigin(pathSurface.PositionWorld, pathSurface.NormalWorld, sample.DirectionWorld),
		        sample.DirectionWorld);
		const float3 hitIncidentRadiance =
		    hitSurface.Valid ? ShadeRayTracingHitIncidentRadiance(hitSurface, sample.DirectionWorld)
		                     : SampleSkyEnvironmentRadiance(SkyTexture, SamplerLinearClamp, sample.DirectionWorld);

		pathContribution += hitIncidentRadiance * pathThroughput;

		if (bounceIndex == 0u)
		{
			firstSample = sample;
			firstTrace = trace;
			firstHitSurface = hitSurface;
			firstHitRadiance = hitIncidentRadiance;
		}

		if (!hitSurface.Valid)
		{
			break;
		}

		pathSurface = BuildHitRayTracingPathSurface(hitSurface, sample.DirectionWorld);
	}

	IndirectSpecularResolvedContribution resolved;
	resolved.HitRadiance = firstHitRadiance;
	resolved.FinalContribution = pathContribution;
	return resolved;
}

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

	const int3 pixel = int3(dispatchThreadId.xy, 0);
	const float deviceZ = LoadGBufferDeviceZ(dispatchThreadId.xy);
	if (IsSkyPixel(deviceZ))
	{
		IndirectSpecularTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		return;
	}

	const float3 baseColor = saturate(GBufferBaseColor.Load(pixel).rgb);
	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(pixel).xyz);
	const float4 materialSample = GBufferMaterial.Load(pixel);
	const float roughness = saturate(materialSample.g);
	const float metallic = saturate(materialSample.r);
	const float dielectricF0 = DecodeGBufferDielectricF0(materialSample.a);
	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, deviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 viewDirWorld = normalize(Camera.Position - positionWorld);
	const float3 mirrorDirectionWorld = normalize(reflect(-viewDirWorld, normalWorld));
	BRDF::SpecularSampling::LobeSample sample;
	RayTracingTraceResult trace;
	RayTracingHitSurfaceData hitSurface;
	const IndirectSpecularResolvedContribution resolved =
	    ResolveIndirectSpecularPath(
	        dispatchThreadId.xy,
	        positionWorld,
	        normalWorld,
	        viewDirWorld,
	        baseColor,
	        roughness,
	        metallic,
	        dielectricF0,
	        sample,
	        trace,
	        hitSurface);
	const float3 debugColor =
	    BuildIndirectSpecularDebugColor(trace, hitSurface, sample, resolved, mirrorDirectionWorld) *
	    lerp(0.65f.xxx, baseColor, 0.35f);
	const float3 reflectionColor = IndirectSpecularDebugMode == RayTracingDebugModes::Off ? resolved.FinalContribution : debugColor;
	const float bindingKeepAliveSignal = float(FrameIndex & 1u) * 1.0e-6f + roughness * 1.0e-9f;

	IndirectSpecularTexture[dispatchThreadId.xy] = float4(reflectionColor, hitSurface.Valid ? 1.0f : bindingKeepAliveSignal);
}
