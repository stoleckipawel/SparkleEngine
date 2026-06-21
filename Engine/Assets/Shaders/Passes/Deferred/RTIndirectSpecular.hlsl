#include "Resources/ConstantBuffers.hlsli"
#include "Common/Random.hlsli"
#include "Passes/Deferred/DirectLightingCommon.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> IndirectSpecularTexture;
RaytracingAccelerationStructure SceneTlas;

cbuffer RTIndirectSpecularUniformData
{
	uint RTIndirectSpecularDebugMode;
	uint RTIndirectSpecularHitDataAvailable;
	float RTIndirectSpecularNormalBias;
	float RTIndirectSpecularMaxDistance;
	uint RTIndirectSpecularHitInstanceCount;
	uint RTIndirectSpecularHitMaterialCount;
	uint RTIndirectSpecularSampleMode;
	uint RTIndirectSpecularPadding0;
};

struct RTIndirectSpecularHitVertex
{
	float3 Position;
	float3 Normal;
	float4 Tangent;
	float2 TexCoord0;
	float2 Padding0;
};

struct RTIndirectSpecularHitInstance
{
	uint FirstVertex;
	uint FirstIndex;
	uint VertexCount;
	uint IndexCount;
	uint MaterialSlot;
	uint Flags;
	uint GeometryFlags;
	uint FallbackReason;
	uint AlphaMode;
	uint MaterialTextureFlags;
	uint AbiVersion;
	uint Padding0;
};

struct RTIndirectSpecularHitMaterial
{
	float4 BaseColor;
	float3 EmissiveColor;
	float Metallic;
	float Roughness;
	float F0;
	float AlphaCutoff;
	uint AlphaMode;
	uint TextureFlags;
	float3 SubsurfaceColor;
	float SubsurfaceStrength;
	uint Flags;
	float2 Padding0;
};

StructuredBuffer<RTIndirectSpecularHitVertex> RTIndirectSpecularHitVertices;
StructuredBuffer<uint> RTIndirectSpecularHitIndices;
StructuredBuffer<RTIndirectSpecularHitInstance> RTIndirectSpecularHitInstances;
StructuredBuffer<RTIndirectSpecularHitMaterial> RTIndirectSpecularHitMaterials;

static const uint RTIndirectSpecularRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint RTIndirectSpecularInstanceMask = 0xFFu;
static const uint RTIndirectSpecularDebugOff = 0u;
static const uint RTIndirectSpecularDebugHitMask = 1u;
static const uint RTIndirectSpecularDebugHitDistance = 2u;
static const uint RTIndirectSpecularDebugMirrorDirection = 3u;
static const uint RTIndirectSpecularDebugHitUV = 4u;
static const uint RTIndirectSpecularDebugHitNormal = 5u;
static const uint RTIndirectSpecularDebugMaterialId = 6u;
static const uint RTIndirectSpecularDebugGeometryClass = 7u;
static const uint RTIndirectSpecularDebugFallbackReason = 8u;
static const uint RTIndirectSpecularDebugAlphaPolicy = 9u;
static const uint RTIndirectSpecularDebugSampleDirection = 10u;
static const uint RTIndirectSpecularDebugSamplePdf = 11u;
static const uint RTIndirectSpecularDebugSampleThroughput = 12u;
static const uint RTIndirectSpecularDebugHitRadiance = 13u;
static const uint RTIndirectSpecularDebugFinalContribution = 14u;
static const uint RTIndirectSpecularSampleModeMirror = 0u;
static const uint RTIndirectSpecularSampleModeStochasticGGX = 1u;
static const uint RTIndirectSpecularHitInstanceFlagValid = 1u << 0u;
static const uint RTIndirectSpecularHitInstanceFlagTwoSided = 1u << 2u;
static const uint RTIndirectSpecularHitGeometryFlagStaticMesh = 1u << 0u;
static const uint RTIndirectSpecularHitGeometryFlagSkinnedMesh = 1u << 1u;
static const uint RTIndirectSpecularHitGeometryFlagAlphaTested = 1u << 2u;
static const uint RTIndirectSpecularHitGeometryFlagAlphaBlended = 1u << 3u;
static const uint RTIndirectSpecularHitGeometryFlagTexturedMaterial = 1u << 4u;
static const uint RTIndirectSpecularHitGeometryFlagDoubleSided = 1u << 5u;
static const uint RTIndirectSpecularHitFallbackReasonNone = 0u;
static const uint RTIndirectSpecularHitFallbackReasonNoHit = 1u;
static const uint RTIndirectSpecularHitFallbackReasonHitDataUnavailable = 2u;
static const uint RTIndirectSpecularHitFallbackReasonInstanceOutOfRange = 3u;
static const uint RTIndirectSpecularHitFallbackReasonInvalidInstance = 4u;
static const uint RTIndirectSpecularHitFallbackReasonInvalidMaterial = 5u;
static const uint RTIndirectSpecularHitFallbackReasonUnsupportedSkinned = 6u;
static const uint RTIndirectSpecularHitFallbackReasonUnsupportedAlphaMode = 7u;
static const uint RTIndirectSpecularHitFallbackReasonMissingMeshHitData = 8u;
static const uint RTIndirectSpecularHitFallbackReasonInvalidPrimitive = 9u;
static const uint RTIndirectSpecularHitFallbackReasonInvalidVertexIndex = 10u;
static const float RTIndirectSpecularMinimumTMin = 0.001f;

struct RTIndirectSpecularTraceResult
{
	bool Hit;
	float RayT;
	uint InstanceId;
	uint PrimitiveIndex;
	float2 Barycentrics;
};

struct RTIndirectSpecularHitSurface
{
	bool Valid;
	float3 PositionWorld;
	float3 NormalWorld;
	float3 TangentWorld;
	float TangentSign;
	float2 TexCoord0;
	uint MaterialSlot;
	uint GeometryFlags;
	uint FallbackReason;
	float3 BaseColor;
	float3 EmissiveColor;
	float3 SubsurfaceColor;
	float Roughness;
	float Metallic;
	float DielectricF0;
	float Alpha;
	float SubsurfaceStrength;
};

struct RTIndirectSpecularSampleResult
{
	float3 DirectionWorld;
	float Pdf;
	float ThroughputNoF;
	bool Mirror;
};

struct RTIndirectSpecularResolvedContribution
{
	float3 HitRadiance;
	float3 FinalContribution;
};

float3 HashIdColor(uint instanceId, uint primitiveIndex)
{
	const uint seed = instanceId * 1664525u + primitiveIndex * 1013904223u + 0x9E3779B9u;
	const uint r = (seed >> 0u) & 255u;
	const uint g = (seed >> 8u) & 255u;
	const uint b = (seed >> 16u) & 255u;
	return 0.15f.xxx + 0.85f * (float3(r, g, b) / 255.0f);
}

float3 SafeNormalize(float3 value, float3 fallback)
{
	const float lengthSquared = dot(value, value);
	return lengthSquared > 1.0e-8f ? value * rsqrt(lengthSquared) : fallback;
}

void BuildOrthonormalBasis(float3 normalWorld, out float3 tangentWorld, out float3 bitangentWorld)
{
	const float sign = normalWorld.z >= 0.0f ? 1.0f : -1.0f;
	const float a = -1.0f / (sign + normalWorld.z);
	const float b = normalWorld.x * normalWorld.y * a;
	tangentWorld = SafeNormalize(float3(1.0f + sign * normalWorld.x * normalWorld.x * a, sign * b, -sign * normalWorld.x), float3(1.0f, 0.0f, 0.0f));
	bitangentWorld = SafeNormalize(float3(b, sign + normalWorld.y * normalWorld.y * a, -normalWorld.y), float3(0.0f, 1.0f, 0.0f));
}

float2 BuildReflectionRandomSample(uint2 pixelCoord)
{
	return CommonRandom::InterleavedGradientNoise2(float2(pixelCoord), FrameIndex, float2(41.0f, 137.0f));
}

float3 SampleGGXHalfVector(float3 normalWorld, float roughness, float2 sample)
{
	float3 tangentWorld;
	float3 bitangentWorld;
	BuildOrthonormalBasis(normalWorld, tangentWorld, bitangentWorld);

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

RTIndirectSpecularSampleResult BuildReflectionSample(uint2 pixelCoord, float3 normalWorld, float3 viewDirWorld, float roughness)
{
	RTIndirectSpecularSampleResult result;
	const float safeRoughness = max(roughness, 1.0e-4f);
	const float3 mirrorDirection = SafeNormalize(reflect(-viewDirWorld, normalWorld), normalWorld);
	const bool forceMirror =
	    RTIndirectSpecularSampleMode == RTIndirectSpecularSampleModeMirror || roughness <= 1.0e-4f;

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

RTIndirectSpecularTraceResult TraceMirrorReflection(float3 positionWorld, float3 normalWorld, float3 reflectionDirectionWorld)
{
	RayDesc ray;
	ray.Origin = positionWorld + normalWorld * max(RTIndirectSpecularNormalBias, 0.0f);
	ray.Direction = normalize(reflectionDirectionWorld);
	ray.TMin = RTIndirectSpecularMinimumTMin;
	ray.TMax = max(RTIndirectSpecularMaxDistance, RTIndirectSpecularMinimumTMin);

	RayQuery<RTIndirectSpecularRayFlags> query;
	query.TraceRayInline(SceneTlas, RTIndirectSpecularRayFlags, RTIndirectSpecularInstanceMask, ray);
	while (query.Proceed())
	{
	}

	RTIndirectSpecularTraceResult result;
	result.Hit = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
	result.RayT = result.Hit ? query.CommittedRayT() : ray.TMax;
	result.InstanceId = result.Hit ? query.CommittedInstanceID() : 0u;
	result.PrimitiveIndex = result.Hit ? query.CommittedPrimitiveIndex() : 0u;
	result.Barycentrics = result.Hit ? query.CommittedTriangleBarycentrics() : 0.0f.xx;
	return result;
}

RTIndirectSpecularHitSurface ReconstructHitSurface(RTIndirectSpecularTraceResult trace, float3 rayOriginWorld, float3 rayDirectionWorld)
{
	RTIndirectSpecularHitSurface surface;
	surface.Valid = false;
	surface.PositionWorld = rayOriginWorld + rayDirectionWorld * trace.RayT;
	surface.NormalWorld = 0.0f.xxx;
	surface.TangentWorld = 0.0f.xxx;
	surface.TangentSign = 1.0f;
	surface.TexCoord0 = 0.0f.xx;
	surface.MaterialSlot = 0u;
	surface.GeometryFlags = 0u;
	surface.FallbackReason = trace.Hit ? RTIndirectSpecularHitFallbackReasonHitDataUnavailable : RTIndirectSpecularHitFallbackReasonNoHit;
	surface.BaseColor = 0.0f.xxx;
	surface.EmissiveColor = 0.0f.xxx;
	surface.SubsurfaceColor = 0.0f.xxx;
	surface.Roughness = 1.0f;
	surface.Metallic = 0.0f;
	surface.DielectricF0 = 0.04f;
	surface.Alpha = 1.0f;
	surface.SubsurfaceStrength = 0.0f;

	if (!trace.Hit)
	{
		return surface;
	}
	if (RTIndirectSpecularHitDataAvailable == 0u)
	{
		surface.FallbackReason = RTIndirectSpecularHitFallbackReasonHitDataUnavailable;
		return surface;
	}
	if (trace.InstanceId >= RTIndirectSpecularHitInstanceCount)
	{
		surface.FallbackReason = RTIndirectSpecularHitFallbackReasonInstanceOutOfRange;
		return surface;
	}

	const RTIndirectSpecularHitInstance hitInstance = RTIndirectSpecularHitInstances[trace.InstanceId];
	surface.MaterialSlot = hitInstance.MaterialSlot;
	surface.GeometryFlags = hitInstance.GeometryFlags;
	surface.FallbackReason = hitInstance.FallbackReason;
	if ((hitInstance.Flags & RTIndirectSpecularHitInstanceFlagValid) == 0u)
	{
		return surface;
	}
	if (hitInstance.MaterialSlot >= RTIndirectSpecularHitMaterialCount)
	{
		surface.FallbackReason = RTIndirectSpecularHitFallbackReasonInvalidMaterial;
		return surface;
	}

	const uint primitiveFirstLocalIndex = trace.PrimitiveIndex * 3u;
	if (primitiveFirstLocalIndex + 2u >= hitInstance.IndexCount)
	{
		surface.FallbackReason = RTIndirectSpecularHitFallbackReasonInvalidPrimitive;
		return surface;
	}

	const uint i0 = hitInstance.FirstVertex + RTIndirectSpecularHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 0u];
	const uint i1 = hitInstance.FirstVertex + RTIndirectSpecularHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 1u];
	const uint i2 = hitInstance.FirstVertex + RTIndirectSpecularHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 2u];
	const uint vertexEnd = hitInstance.FirstVertex + hitInstance.VertexCount;
	if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd)
	{
		surface.FallbackReason = RTIndirectSpecularHitFallbackReasonInvalidVertexIndex;
		return surface;
	}

	const float3 barycentricWeights = float3(1.0f - trace.Barycentrics.x - trace.Barycentrics.y, trace.Barycentrics.x, trace.Barycentrics.y);
	const RTIndirectSpecularHitVertex v0 = RTIndirectSpecularHitVertices[i0];
	const RTIndirectSpecularHitVertex v1 = RTIndirectSpecularHitVertices[i1];
	const RTIndirectSpecularHitVertex v2 = RTIndirectSpecularHitVertices[i2];
	const float3 localNormal = v0.Normal * barycentricWeights.x + v1.Normal * barycentricWeights.y + v2.Normal * barycentricWeights.z;
	const float4 localTangent =
	    v0.Tangent * barycentricWeights.x + v1.Tangent * barycentricWeights.y + v2.Tangent * barycentricWeights.z;
	const MeshInstanceData meshInstance = MeshInstances[trace.InstanceId];
	const float3x3 worldInvTransposeMatrix = (float3x3) meshInstance.WorldInvTransposeMTX;
	const float3x3 worldMatrix = (float3x3) meshInstance.WorldMTX;
	const RTIndirectSpecularHitMaterial material = RTIndirectSpecularHitMaterials[hitInstance.MaterialSlot];
	float3 normalWorld = normalize(mul(localNormal, worldInvTransposeMatrix));
	float3 tangentWorld = normalize(mul(localTangent.xyz, worldMatrix));
	if ((hitInstance.Flags & RTIndirectSpecularHitInstanceFlagTwoSided) != 0u && dot(normalWorld, -rayDirectionWorld) < 0.0f)
	{
		normalWorld = -normalWorld;
		tangentWorld = -tangentWorld;
	}

	surface.Valid = true;
	surface.PositionWorld =
	    mul(float4(v0.Position * barycentricWeights.x + v1.Position * barycentricWeights.y + v2.Position * barycentricWeights.z, 1.0f),
	        meshInstance.WorldMTX)
	        .xyz;
	surface.NormalWorld = normalWorld;
	surface.TangentWorld = tangentWorld;
	surface.TangentSign = localTangent.w >= 0.0f ? 1.0f : -1.0f;
	surface.TexCoord0 = v0.TexCoord0 * barycentricWeights.x + v1.TexCoord0 * barycentricWeights.y + v2.TexCoord0 * barycentricWeights.z;
	surface.MaterialSlot = hitInstance.MaterialSlot;
	surface.GeometryFlags = hitInstance.GeometryFlags;
	surface.FallbackReason = RTIndirectSpecularHitFallbackReasonNone;
	surface.BaseColor = saturate(material.BaseColor.rgb);
	surface.EmissiveColor = max(material.EmissiveColor, 0.0f.xxx);
	surface.SubsurfaceColor = saturate(material.SubsurfaceColor);
	surface.Roughness = saturate(material.Roughness);
	surface.Metallic = saturate(material.Metallic);
	surface.DielectricF0 = saturate(material.F0) * 0.08f;
	surface.Alpha = saturate(material.BaseColor.a);
	surface.SubsurfaceStrength = saturate(material.SubsurfaceStrength);
	return surface;
}

void AccumulateHitDirectLight(
    RTIndirectSpecularHitSurface surface,
    float3 viewDirWorld,
    float3 lightDirection,
    float3 radiance,
    inout float3 incidentRadiance)
{
	BRDF::ShadingData shadingData = BRDF::ComputeShadingData(surface.NormalWorld, viewDirWorld, lightDirection);
	if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f)
	{
		return;
	}

	float3 diffuse = 0.0f.xxx;
	float3 specular = 0.0f.xxx;
	float3 subsurface = 0.0f.xxx;
	const float safeRoughness = max(surface.Roughness, 1.0e-4f);
	const float3 f0 = lerp(surface.DielectricF0.xxx, surface.BaseColor, surface.Metallic);
	BRDF::Direct::Evaluate(
	    shadingData,
	    surface.BaseColor,
	    safeRoughness,
	    surface.Metallic,
	    f0,
	    surface.SubsurfaceColor,
	    surface.SubsurfaceStrength,
	    diffuse,
	    specular,
	    subsurface);

	incidentRadiance += (diffuse + specular + subsurface) * radiance * shadingData.NoL;
}

float3 ShadeHitIncidentRadiance(RTIndirectSpecularHitSurface surface, float3 rayDirectionWorld)
{
	if (!surface.Valid)
	{
		return 0.0f.xxx;
	}

	float3 incidentRadiance = max(surface.EmissiveColor, 0.0f.xxx);
	const float3 viewDirWorld = normalize(-rayDirectionWorld);
	const uint directionalLightCount = min(ViewLighting.DirectionalLightCount, MAX_DIRECTIONAL_LIGHTS);
	const uint pointLightCount = min(ViewLighting.PointLightCount, MAX_POINT_LIGHTS);
	const uint spotLightCount = min(ViewLighting.SpotLightCount, MAX_SPOT_LIGHTS);

	[loop] for (uint lightIndex = 0u; lightIndex < directionalLightCount; ++lightIndex)
	{
		const float3 lightDirection = DirectLighting::GetDirectionalLightDirection(lightIndex);
		const float3 radiance = ViewLighting.DirectionalLights[lightIndex].Color * ViewLighting.DirectionalLights[lightIndex].Intensity;
		AccumulateHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < pointLightCount; ++lightIndex)
	{
		const PointLightConstantBufferData light = ViewLighting.PointLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = DirectLighting::GetPointLightDirection(surface.PositionWorld, lightIndex, distanceToLight);
		const float attenuation = DirectLighting::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float3 radiance = light.Color * light.Intensity * attenuation;
		AccumulateHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < spotLightCount; ++lightIndex)
	{
		const SpotLightConstantBufferData light = ViewLighting.SpotLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = DirectLighting::GetSpotLightDirection(surface.PositionWorld, lightIndex, distanceToLight);
		const float3 lightToSurfaceDirection = -lightDirection;
		const float distanceAttenuation = DirectLighting::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float coneAttenuation =
		    DirectLighting::ComputeSpotConeAttenuation(lightToSurfaceDirection, light.Direction, light.InnerConeCosine, light.OuterConeCosine);
		const float3 radiance = light.Color * light.Intensity * distanceAttenuation * coneAttenuation;
		AccumulateHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	return max(incidentRadiance, 0.0f.xxx);
}

float3 FallbackReasonColor(uint reason)
{
	if (reason == RTIndirectSpecularHitFallbackReasonNone)
	{
		return float3(0.1f, 1.0f, 0.25f);
	}
	if (reason == RTIndirectSpecularHitFallbackReasonNoHit)
	{
		return 0.0f.xxx;
	}
	if (reason == RTIndirectSpecularHitFallbackReasonUnsupportedSkinned)
	{
		return float3(0.65f, 0.2f, 1.0f);
	}
	if (reason == RTIndirectSpecularHitFallbackReasonUnsupportedAlphaMode)
	{
		return float3(1.0f, 0.1f, 0.05f);
	}
	if (reason == RTIndirectSpecularHitFallbackReasonMissingMeshHitData)
	{
		return float3(1.0f, 0.55f, 0.0f);
	}
	if (reason == RTIndirectSpecularHitFallbackReasonInvalidMaterial)
	{
		return float3(1.0f, 0.0f, 0.8f);
	}
	if (reason == RTIndirectSpecularHitFallbackReasonInvalidPrimitive || reason == RTIndirectSpecularHitFallbackReasonInvalidVertexIndex)
	{
		return float3(1.0f, 1.0f, 0.0f);
	}
	return float3(0.2f, 0.65f, 1.0f);
}

float3 GeometryClassColor(uint geometryFlags)
{
	if ((geometryFlags & RTIndirectSpecularHitGeometryFlagSkinnedMesh) != 0u)
	{
		return float3(0.65f, 0.2f, 1.0f);
	}
	if ((geometryFlags & RTIndirectSpecularHitGeometryFlagAlphaTested) != 0u ||
	    (geometryFlags & RTIndirectSpecularHitGeometryFlagAlphaBlended) != 0u)
	{
		return float3(1.0f, 0.2f, 0.05f);
	}
	if ((geometryFlags & RTIndirectSpecularHitGeometryFlagDoubleSided) != 0u)
	{
		return float3(0.05f, 0.85f, 1.0f);
	}
	if ((geometryFlags & RTIndirectSpecularHitGeometryFlagTexturedMaterial) != 0u)
	{
		return float3(0.1f, 0.95f, 0.45f);
	}
	if ((geometryFlags & RTIndirectSpecularHitGeometryFlagStaticMesh) != 0u)
	{
		return float3(0.85f, 0.85f, 0.85f);
	}
	return float3(0.2f, 0.2f, 0.2f);
}

float3 PreviewHdr(float3 value)
{
	return value / (1.0f.xxx + value);
}

float3 BuildMirrorDebugColor(
    RTIndirectSpecularTraceResult trace,
    RTIndirectSpecularHitSurface hitSurface,
    RTIndirectSpecularSampleResult sample,
    RTIndirectSpecularResolvedContribution resolved,
    float3 mirrorDirectionWorld)
{
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugMirrorDirection)
	{
		return mirrorDirectionWorld * 0.5f + 0.5f;
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugSampleDirection)
	{
		return sample.DirectionWorld * 0.5f + 0.5f;
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugSamplePdf)
	{
		return saturate(sample.Pdf * 4.0f).xxx;
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugSampleThroughput)
	{
		return saturate(sample.ThroughputNoF).xxx;
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugHitRadiance)
	{
		return PreviewHdr(resolved.HitRadiance);
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugFinalContribution)
	{
		return PreviewHdr(resolved.FinalContribution);
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugFallbackReason)
	{
		return FallbackReasonColor(hitSurface.FallbackReason);
	}

	if (!trace.Hit)
	{
		return 0.0f.xxx;
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugHitMask)
	{
		return 1.0f.xxx;
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugHitDistance)
	{
		const float normalizedDistance = saturate(trace.RayT / max(RTIndirectSpecularMaxDistance, RTIndirectSpecularMinimumTMin));
		return lerp(float3(0.05f, 0.25f, 1.0f), float3(1.0f, 0.85f, 0.05f), normalizedDistance);
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugHitUV)
	{
		return hitSurface.Valid ? float3(frac(hitSurface.TexCoord0), 0.0f) : FallbackReasonColor(hitSurface.FallbackReason);
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugHitNormal)
	{
		return hitSurface.Valid ? hitSurface.NormalWorld * 0.5f + 0.5f : FallbackReasonColor(hitSurface.FallbackReason);
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugMaterialId)
	{
		return hitSurface.Valid ? HashIdColor(hitSurface.MaterialSlot, 0u) : FallbackReasonColor(hitSurface.FallbackReason);
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugGeometryClass)
	{
		return GeometryClassColor(hitSurface.GeometryFlags);
	}

	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugAlphaPolicy)
	{
		return hitSurface.FallbackReason == RTIndirectSpecularHitFallbackReasonUnsupportedAlphaMode
		           ? float3(1.0f, 0.0f, 0.0f)
		           : GeometryClassColor(hitSurface.GeometryFlags);
	}

	const float3 idColor = HashIdColor(trace.InstanceId, trace.PrimitiveIndex);
	const float distanceShade = 1.0f / (1.0f + trace.RayT * 0.02f);
	return idColor * distanceShade;
}

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
	const RTIndirectSpecularSampleResult sample =
	    BuildReflectionSample(dispatchThreadId.xy, normalWorld, viewDirWorld, roughness);
	const RTIndirectSpecularTraceResult trace =
	    TraceMirrorReflection(positionWorld, normalWorld, sample.DirectionWorld);
	const RTIndirectSpecularHitSurface hitSurface =
	    ReconstructHitSurface(trace, positionWorld + normalWorld * max(RTIndirectSpecularNormalBias, 0.0f), sample.DirectionWorld);
	const float3 hitIncidentRadiance = hitSurface.Valid ? ShadeHitIncidentRadiance(hitSurface, sample.DirectionWorld) : 0.0f.xxx;
	RTIndirectSpecularResolvedContribution resolved;
	resolved.HitRadiance = hitIncidentRadiance;
	resolved.FinalContribution = hitIncidentRadiance * sample.ThroughputNoF;
	const float3 debugColor =
	    BuildMirrorDebugColor(trace, hitSurface, sample, resolved, mirrorDirectionWorld) * lerp(0.65f.xxx, baseColor, 0.35f);
	const float3 reflectionColor = RTIndirectSpecularDebugMode == RTIndirectSpecularDebugOff ? resolved.FinalContribution : debugColor;
	const float bindingKeepAliveSignal = float(FrameIndex & 1u) * 1.0e-6f + roughness * 1.0e-9f;

	IndirectSpecularTexture[dispatchThreadId.xy] = float4(reflectionColor, hitSurface.Valid ? 1.0f : bindingKeepAliveSignal);
}
