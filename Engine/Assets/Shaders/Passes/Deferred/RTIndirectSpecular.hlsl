#include "Resources/ConstantBuffers.hlsli"
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
	uint RTIndirectSpecularPadding0;
	uint RTIndirectSpecularPadding1;
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
	float Roughness;
	float Metallic;
};

float3 HashIdColor(uint instanceId, uint primitiveIndex)
{
	const uint seed = instanceId * 1664525u + primitiveIndex * 1013904223u + 0x9E3779B9u;
	const uint r = (seed >> 0u) & 255u;
	const uint g = (seed >> 8u) & 255u;
	const uint b = (seed >> 16u) & 255u;
	return 0.15f.xxx + 0.85f * (float3(r, g, b) / 255.0f);
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
	surface.Roughness = 1.0f;
	surface.Metallic = 0.0f;

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
	surface.Roughness = saturate(material.Roughness);
	surface.Metallic = saturate(material.Metallic);
	return surface;
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

float3 BuildMirrorDebugColor(RTIndirectSpecularTraceResult trace, RTIndirectSpecularHitSurface hitSurface, float3 reflectionDirectionWorld)
{
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugMirrorDirection)
	{
		return reflectionDirectionWorld * 0.5f + 0.5f;
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
	const float3 reflectionDirectionWorld = normalize(reflect(-viewDirWorld, normalWorld));
	const RTIndirectSpecularTraceResult trace =
	    TraceMirrorReflection(positionWorld, normalWorld, reflectionDirectionWorld);
	const RTIndirectSpecularHitSurface hitSurface =
	    ReconstructHitSurface(trace, positionWorld + normalWorld * max(RTIndirectSpecularNormalBias, 0.0f), reflectionDirectionWorld);
	const float3 debugColor = BuildMirrorDebugColor(trace, hitSurface, reflectionDirectionWorld) * lerp(0.65f.xxx, baseColor, 0.35f);
	const float NoV = hitSurface.Valid ? saturate(dot(hitSurface.NormalWorld, -reflectionDirectionWorld)) : 0.0f;
	const float3 hitMaterialColor =
	    hitSurface.Valid
	        ? hitSurface.BaseColor * lerp(0.35f, 1.0f, NoV) * lerp(1.0f, 0.65f, hitSurface.Roughness)
	        : debugColor;
	const float3 reflectionColor = RTIndirectSpecularDebugMode == RTIndirectSpecularDebugOff ? hitMaterialColor : debugColor;
	const float bindingKeepAliveSignal = float(FrameIndex & 1u) * 1.0e-6f + roughness * 1.0e-9f;

	IndirectSpecularTexture[dispatchThreadId.xy] = float4(reflectionColor, hitSurface.Valid ? 1.0f : bindingKeepAliveSignal);
}
