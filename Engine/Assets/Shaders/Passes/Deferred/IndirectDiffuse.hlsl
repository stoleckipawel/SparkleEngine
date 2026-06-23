#include "Resources/ConstantBuffers.hlsli"
#include "Common/Random.hlsli"
#include "Common/Sampling.hlsli"
#include "Lighting/SkyEnvironment.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/RayTracingPathSample.hlsli"

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
	uint IndirectDiffusePadding0;
	uint IndirectDiffusePadding1;
};

#include "RayTracing/RayTracingHitLighting.hlsli"
#include "Passes/Deferred/IndirectDiffuseDebug.hlsli"

static const uint IndirectDiffuseRayFlags = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint IndirectDiffuseInstanceMask = 0xFFu;
static const float IndirectDiffuseMinimumTMin = 0.001f;

float2 BuildIndirectDiffuseRandomSample(uint2 pixelCoord)
{
	return CommonRandom::InterleavedGradientNoise2(float2(pixelCoord), FrameIndex, float2(211.0f, 97.0f));
}

RayTracingPathSample::DirectionSample BuildIndirectDiffuseDirectionSample(uint2 pixelCoord, float3 normalWorld)
{
	const CommonSampling::CosineHemisphereSample cosineSample =
	    CommonSampling::SampleCosineHemisphere(normalWorld, BuildIndirectDiffuseRandomSample(pixelCoord));

	RayTracingPathSample::DirectionSample result;
	result.DirectionWorld = cosineSample.DirectionWorld;
	result.Pdf = cosineSample.Pdf;
	result.CosineTerm = cosineSample.Cosine;
	result.RejectionReason = result.Pdf > 0.0f && result.CosineTerm > 0.0f
	                             ? RayTracingPathSample::RejectionReasonNone
	                             : RayTracingPathSample::RejectionReasonInvalidSample;
	return result;
}

float3 ComputeIndirectDiffuseRayOrigin(float3 positionWorld, float3 normalWorld, float3 rayDirectionWorld)
{
	const float bias = max(IndirectDiffuseNormalBias, 0.0f);
	const float NoR = abs(dot(normalWorld, rayDirectionWorld));
	const float grazingScale = rcp(max(NoR, 0.25f));
	return positionWorld + normalWorld * bias * grazingScale + rayDirectionWorld * IndirectDiffuseMinimumTMin;
}

RayTracingTraceResult TraceIndirectDiffuseRay(float3 rayOriginWorld, float3 rayDirectionWorld)
{
	RayDesc ray;
	ray.Direction = normalize(rayDirectionWorld);
	ray.Origin = rayOriginWorld;
	ray.TMin = IndirectDiffuseMinimumTMin;
	ray.TMax = max(IndirectDiffuseMaxDistance, IndirectDiffuseMinimumTMin);

	RayQuery<IndirectDiffuseRayFlags> query;
	query.TraceRayInline(SceneTlas, IndirectDiffuseRayFlags, IndirectDiffuseInstanceMask, ray);
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

RayTracingPathSample::LightingResult ResolveIndirectDiffuseLighting(
    RayTracingTraceResult trace,
    RayTracingPathSample::DirectionSample sample,
    float3 rayOriginWorld)
{
	RayTracingPathSample::LightingResult result;
	result.TraceHit = trace.Hit;
	result.Hit = trace.Hit;
	result.HitDistance = trace.RayT;
	result.RejectionReason = sample.RejectionReason;
	result.IncidentRadiance = 0.0f.xxx;
	result.Contribution = 0.0f.xxx;
	result.HitNormalWorld = 0.0f.xxx;
	result.MaterialBaseColor = 0.0f.xxx;
	result.MissRadiance = 0.0f.xxx;
	result.SurfaceRejectionReason = trace.Hit ? RayTracingHitSurface::ReasonHitDataUnavailable : RayTracingHitSurface::ReasonNoHit;

	if (sample.RejectionReason != RayTracingPathSample::RejectionReasonNone)
	{
		return result;
	}

	if (trace.Hit)
	{
		const RayTracingHitSurfaceData hitSurface =
		    ReconstructRayTracingHitSurface(trace, rayOriginWorld, sample.DirectionWorld);
		result.Hit = hitSurface.Valid;
		result.RejectionReason = hitSurface.Valid
		                             ? RayTracingPathSample::RejectionReasonNone
		                             : RayTracingPathSample::RejectionReasonHitSurfaceRejected;
		result.SurfaceRejectionReason = hitSurface.RejectionReason;
		result.HitNormalWorld = hitSurface.Valid ? hitSurface.NormalWorld : 0.0f.xxx;
		result.MaterialBaseColor = hitSurface.Valid ? hitSurface.BaseColor : 0.0f.xxx;
		result.IncidentRadiance = hitSurface.Valid
		                              ? ShadeRayTracingHitIncidentRadiance(hitSurface, sample.DirectionWorld)
		                              : 0.0f.xxx;
	}
	else
	{
		result.RejectionReason = RayTracingPathSample::RejectionReasonTraceMiss;
		result.SurfaceRejectionReason = RayTracingHitSurface::ReasonNoHit;
		result.MissRadiance = SampleSkyEnvironment(SkyTexture, SamplerLinearClamp, sample.DirectionWorld);
		result.IncidentRadiance = result.MissRadiance;
	}

	result.Contribution = result.IncidentRadiance * (sample.CosineTerm * rcp(max(sample.Pdf, 1.0e-4f)) * INV_PI);
	return result;
}

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
	const float3 baseColor = saturate(GBufferBaseColor.Load(pixel).rgb);
	const float roughness = saturate(GBufferMaterial.Load(pixel).g);
	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(pixelCoord, deviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const RayTracingPathSample::DirectionSample sample =
	    BuildIndirectDiffuseDirectionSample(pixelCoord, normalWorld);
	const float3 rayOriginWorld = ComputeIndirectDiffuseRayOrigin(positionWorld, normalWorld, sample.DirectionWorld);
	const RayTracingTraceResult trace = TraceIndirectDiffuseRay(rayOriginWorld, sample.DirectionWorld);
	const RayTracingPathSample::LightingResult lighting =
	    ResolveIndirectDiffuseLighting(trace, sample, rayOriginWorld);
	const float bindingKeepAliveSignal = dot(baseColor, float3(0.25f, 0.5f, 0.25f)) * 1.0e-9f + roughness * 1.0e-9f;
	const float alphaSignal = (lighting.Hit ? 1.0f : 0.0f) + bindingKeepAliveSignal;
	const float3 finalContribution = lighting.Contribution * IndirectDiffuseIntensity;
	const float3 outputColor = IndirectDiffuseDebugMode == IndirectDiffuseDebug::Off
	                               ? finalContribution
	                               : IndirectDiffuseDebug::BuildColor(
	                                     IndirectDiffuseDebugMode,
	                                     sample,
	                                     lighting,
	                                     IndirectDiffuseMaxDistance);

	IndirectDiffuseTexture[pixelCoord] = float4(outputColor, alphaSignal);
}
