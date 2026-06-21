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
};

struct RTIndirectSpecularHitInstance
{
	uint FirstVertex;
	uint FirstIndex;
	uint VertexCount;
	uint IndexCount;
	uint MaterialSlot;
	uint Flags;
	uint Padding0;
	uint Padding1;
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
	float3 Padding0;
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
static const uint RTIndirectSpecularHitInstanceFlagValid = 1u << 0u;
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
	uint MaterialSlot;
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
	surface.MaterialSlot = 0u;
	surface.BaseColor = 0.0f.xxx;
	surface.Roughness = 1.0f;
	surface.Metallic = 0.0f;

	if (!trace.Hit || RTIndirectSpecularHitDataAvailable == 0u || trace.InstanceId >= RTIndirectSpecularHitInstanceCount)
	{
		return surface;
	}

	const RTIndirectSpecularHitInstance hitInstance = RTIndirectSpecularHitInstances[trace.InstanceId];
	if ((hitInstance.Flags & RTIndirectSpecularHitInstanceFlagValid) == 0u || hitInstance.MaterialSlot >= RTIndirectSpecularHitMaterialCount)
	{
		return surface;
	}

	const uint primitiveFirstLocalIndex = trace.PrimitiveIndex * 3u;
	if (primitiveFirstLocalIndex + 2u >= hitInstance.IndexCount)
	{
		return surface;
	}

	const uint i0 = hitInstance.FirstVertex + RTIndirectSpecularHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 0u];
	const uint i1 = hitInstance.FirstVertex + RTIndirectSpecularHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 1u];
	const uint i2 = hitInstance.FirstVertex + RTIndirectSpecularHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 2u];
	const uint vertexEnd = hitInstance.FirstVertex + hitInstance.VertexCount;
	if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd)
	{
		return surface;
	}

	const float3 barycentricWeights = float3(1.0f - trace.Barycentrics.x - trace.Barycentrics.y, trace.Barycentrics.x, trace.Barycentrics.y);
	const float3 localNormal =
	    RTIndirectSpecularHitVertices[i0].Normal * barycentricWeights.x +
	    RTIndirectSpecularHitVertices[i1].Normal * barycentricWeights.y +
	    RTIndirectSpecularHitVertices[i2].Normal * barycentricWeights.z;
	const MeshInstanceData meshInstance = MeshInstances[trace.InstanceId];
	const float3x3 worldInvTransposeMatrix = (float3x3) meshInstance.WorldInvTransposeMTX;
	const RTIndirectSpecularHitMaterial material = RTIndirectSpecularHitMaterials[hitInstance.MaterialSlot];

	surface.Valid = true;
	surface.NormalWorld = normalize(mul(localNormal, worldInvTransposeMatrix));
	surface.MaterialSlot = hitInstance.MaterialSlot;
	surface.BaseColor = saturate(material.BaseColor.rgb);
	surface.Roughness = saturate(material.Roughness);
	surface.Metallic = saturate(material.Metallic);
	return surface;
}

float3 BuildMirrorDebugColor(RTIndirectSpecularTraceResult trace, float3 reflectionDirectionWorld)
{
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugMirrorDirection)
	{
		return reflectionDirectionWorld * 0.5f + 0.5f;
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
	const float3 debugColor = BuildMirrorDebugColor(trace, reflectionDirectionWorld) * lerp(0.65f.xxx, baseColor, 0.35f);
	const float NoV = hitSurface.Valid ? saturate(dot(hitSurface.NormalWorld, -reflectionDirectionWorld)) : 0.0f;
	const float3 hitMaterialColor =
	    hitSurface.Valid
	        ? hitSurface.BaseColor * lerp(0.35f, 1.0f, NoV) * lerp(1.0f, 0.65f, hitSurface.Roughness)
	        : debugColor;
	const float3 reflectionColor = RTIndirectSpecularDebugMode == RTIndirectSpecularDebugOff ? hitMaterialColor : debugColor;
	const float bindingKeepAliveSignal = float(FrameIndex & 1u) * 1.0e-6f + roughness * 1.0e-9f;

	IndirectSpecularTexture[dispatchThreadId.xy] = float4(reflectionColor, hitSurface.Valid ? 1.0f : bindingKeepAliveSignal);
}
