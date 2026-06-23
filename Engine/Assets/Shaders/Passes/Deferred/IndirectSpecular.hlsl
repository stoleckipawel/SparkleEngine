#include "Resources/ConstantBuffers.hlsli"
#include "Common/Math.hlsli"
#include "Common/Random.hlsli"
#include "Geometry/Basis.hlsli"
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
	uint IndirectSpecularPadding0;
	uint IndirectSpecularPadding1;
};

#include "RayTracing/RayTracingHitLighting.hlsli"

static const uint IndirectSpecularRayFlags = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint IndirectSpecularInstanceMask = 0xFFu;
static const uint IndirectSpecularDebugMirrorDirection = 3u;
static const uint IndirectSpecularDebugSampleDirection = 10u;
static const uint IndirectSpecularDebugSamplePdf = 11u;
static const uint IndirectSpecularDebugSampleThroughput = 12u;
static const uint IndirectSpecularDebugHitRadiance = 13u;
static const uint IndirectSpecularDebugFinalContribution = 14u;
static const uint IndirectSpecularSampleModeMirror = 0u;
static const uint IndirectSpecularSampleModeStochasticGGX = 1u;
static const float IndirectSpecularMinimumTMin = 0.001f;

struct IndirectSpecularSampleResult
{
	float3 DirectionWorld;
	float Pdf;
	float ThroughputNoF;
	bool Mirror;
};

struct IndirectSpecularResolvedContribution
{
	float3 HitRadiance;
	float3 FinalContribution;
};

float2 BuildReflectionRandomSample(uint2 pixelCoord)
{
	return CommonRandom::InterleavedGradientNoise2(float2(pixelCoord), FrameIndex, float2(41.0f, 137.0f));
}

float3 SampleGGXHalfVector(float3 normalWorld, float roughness, float2 sample)
{
	float3 tangentWorld;
	float3 bitangentWorld;
	CommonSampling::BuildOrthonormalBasis(normalWorld, tangentWorld, bitangentWorld);

	const float alpha = max(roughness * roughness, 1.0e-4f);
	const float alphaSquared = alpha * alpha;
	const float phi = TWO_PI * sample.x;
	const float cosTheta = sqrt(saturate((1.0f - sample.y) / max(1.0f + (alphaSquared - 1.0f) * sample.y, 1.0e-4f)));
	const float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
	const float3 localHalfVector = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	return SafeNormalize(
	    tangentWorld * localHalfVector.x + bitangentWorld * localHalfVector.y + normalWorld * localHalfVector.z,
	    normalWorld);
}

IndirectSpecularSampleResult BuildReflectionSample(uint2 pixelCoord, float3 normalWorld, float3 viewDirWorld, float roughness)
{
	IndirectSpecularSampleResult result;
	const float safeRoughness = max(roughness, 1.0e-4f);
	const float3 mirrorDirection = SafeNormalize(reflect(-viewDirWorld, normalWorld), normalWorld);
	const bool forceMirror =
	    IndirectSpecularSampleMode == IndirectSpecularSampleModeMirror || roughness <= 1.0e-4f;

	result.DirectionWorld = mirrorDirection;
	result.Pdf = 1.0f;
	result.ThroughputNoF = 1.0f;
	result.Mirror = forceMirror;

	if (forceMirror)
	{
		return result;
	}

	const float2 randomSample = BuildReflectionRandomSample(pixelCoord);
	const float3 halfVectorWorld = SampleGGXHalfVector(normalWorld, safeRoughness, randomSample);
	const float3 sampleDirectionWorld = SafeNormalize(reflect(-viewDirWorld, halfVectorWorld), mirrorDirection);
	const float NoV = saturate(dot(normalWorld, viewDirWorld));
	const float NoL = saturate(dot(normalWorld, sampleDirectionWorld));
	const float NoH = saturate(dot(normalWorld, halfVectorWorld));
	const float VoH = saturate(dot(viewDirWorld, halfVectorWorld));

	if (NoV <= 0.0f || NoL <= 0.0f || NoH <= 0.0f || VoH <= 0.0f)
	{
		result.DirectionWorld = mirrorDirection;
		result.Pdf = 1.0f;
		result.ThroughputNoF = 0.0f;
		result.Mirror = true;
		return result;
	}

	const float alpha = safeRoughness * safeRoughness;
	const float D = BRDF::Distribution::Evaluate(NoH, alpha);
	const float pdf = max(D * NoH / max(4.0f * VoH, 1.0e-4f), 1.0e-4f);
	const BRDF::ShadingData shadingData = BRDF::ComputeShadingData(normalWorld, viewDirWorld, sampleDirectionWorld);
	const float3 specularNoF = BRDF::Specular::EvaluateDirect(shadingData, safeRoughness, 1.0f.xxx);
	const float throughputNoF = max(max(specularNoF.r, specularNoF.g), specularNoF.b) * NoL / pdf;

	result.DirectionWorld = sampleDirectionWorld;
	result.Pdf = pdf;
	result.ThroughputNoF = max(throughputNoF, 0.0f);
	result.Mirror = false;
	return result;
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
	RayDesc ray;
	ray.Direction = SafeNormalize(reflectionDirectionWorld, normalWorld);
	ray.Origin = ComputeRayOrigin(positionWorld, normalWorld, ray.Direction);
	ray.TMin = IndirectSpecularMinimumTMin;
	ray.TMax = max(IndirectSpecularMaxDistance, IndirectSpecularMinimumTMin);

	RayQuery<IndirectSpecularRayFlags> query;
	query.TraceRayInline(SceneTlas, IndirectSpecularRayFlags, IndirectSpecularInstanceMask, ray);
	float alphaCandidateValue = 1.0f;
	float alphaCandidateCutoff = 0.5f;
	bool alphaCandidateSeen = false;
	bool alphaCandidateAccepted = false;
	bool alphaCandidateRejected = false;
	while (query.Proceed())
	{
		if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
		{
			alphaCandidateSeen = true;
			const bool commitCandidate = ResolveRayTracingCandidateAlpha(
			    query.CandidateInstanceID(),
			    query.CandidatePrimitiveIndex(),
			    query.CandidateTriangleBarycentrics(),
			    alphaCandidateValue,
			    alphaCandidateCutoff);
			if (commitCandidate)
			{
				alphaCandidateAccepted = true;
				query.CommitNonOpaqueTriangleHit();
			}
			else
			{
				alphaCandidateRejected = true;
			}
		}
	}

	RayTracingTraceResult result;
	result.Hit = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
	result.RayT = result.Hit ? query.CommittedRayT() : ray.TMax;
	result.InstanceId = result.Hit ? query.CommittedInstanceID() : 0u;
	result.PrimitiveIndex = result.Hit ? query.CommittedPrimitiveIndex() : 0u;
	result.Barycentrics = result.Hit ? query.CommittedTriangleBarycentrics() : 0.0f.xx;
	result.AlphaCandidateSeen = alphaCandidateSeen;
	result.AlphaCandidateAccepted = alphaCandidateAccepted && result.Hit;
	result.AlphaCandidateRejected = alphaCandidateRejected;
	result.AlphaCandidateValue = alphaCandidateValue;
	result.AlphaCandidateCutoff = alphaCandidateCutoff;
	return result;
}

float3 SampleIndirectSpecularMissRadiance(float3 rayDirectionWorld)
{
	return SampleSkyEnvironment(SkyTexture, SamplerLinearClamp, rayDirectionWorld);
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
	const float roughness = saturate(GBufferMaterial.Load(pixel).g);
	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, deviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 viewDirWorld = normalize(Camera.Position - positionWorld);
	const float3 mirrorDirectionWorld = normalize(reflect(-viewDirWorld, normalWorld));
	const IndirectSpecularSampleResult sample =
	    BuildReflectionSample(dispatchThreadId.xy, normalWorld, viewDirWorld, roughness);
	const RayTracingTraceResult trace =
	    TraceIndirectSpecularRay(positionWorld, normalWorld, sample.DirectionWorld);
	const RayTracingHitSurfaceData hitSurface =
	    ReconstructRayTracingHitSurface(trace, ComputeRayOrigin(positionWorld, normalWorld, sample.DirectionWorld), sample.DirectionWorld);
	const float3 hitIncidentRadiance =
	    hitSurface.Valid ? ShadeRayTracingHitIncidentRadiance(hitSurface, sample.DirectionWorld) : SampleIndirectSpecularMissRadiance(sample.DirectionWorld);
	IndirectSpecularResolvedContribution resolved;
	resolved.HitRadiance = hitIncidentRadiance;
	resolved.FinalContribution = hitIncidentRadiance * sample.ThroughputNoF;
	const float3 debugColor =
	    BuildIndirectSpecularDebugColor(trace, hitSurface, sample, resolved, mirrorDirectionWorld) *
	    lerp(0.65f.xxx, baseColor, 0.35f);
	const float3 reflectionColor = IndirectSpecularDebugMode == RayTracingDebugModes::Off ? resolved.FinalContribution : debugColor;
	const float bindingKeepAliveSignal = float(FrameIndex & 1u) * 1.0e-6f + roughness * 1.0e-9f;

	IndirectSpecularTexture[dispatchThreadId.xy] = float4(reflectionColor, hitSurface.Valid ? 1.0f : bindingKeepAliveSignal);
}
