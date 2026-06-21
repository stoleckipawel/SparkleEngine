#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> IndirectSpecularTexture;
RaytracingAccelerationStructure SceneTlas;

cbuffer RTIndirectSpecularUniformData
{
	uint RTIndirectSpecularDebugMode;
	float RTIndirectSpecularNormalBias;
	float RTIndirectSpecularMaxDistance;
	float RTIndirectSpecularPadding0;
};

static const uint RTIndirectSpecularRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint RTIndirectSpecularInstanceMask = 0xFFu;
static const uint RTIndirectSpecularDebugOff = 0u;
static const uint RTIndirectSpecularDebugHitMask = 1u;
static const uint RTIndirectSpecularDebugHitDistance = 2u;
static const uint RTIndirectSpecularDebugMirrorDirection = 3u;
static const float RTIndirectSpecularMinimumTMin = 0.001f;

struct RTIndirectSpecularTraceResult
{
	bool Hit;
	float RayT;
	uint InstanceId;
	uint PrimitiveIndex;
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
	return result;
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
	const float3 debugColor = BuildMirrorDebugColor(trace, reflectionDirectionWorld) * lerp(0.65f.xxx, baseColor, 0.35f);
	const float bindingKeepAliveSignal = float(FrameIndex & 1u) * 1.0e-6f + roughness * 1.0e-9f;

	IndirectSpecularTexture[dispatchThreadId.xy] = float4(debugColor, trace.Hit ? 1.0f : bindingKeepAliveSignal);
}
