#include "Resources/ConstantBuffers.hlsli"

RWTexture2D<float4> IndirectSpecularTexture;
RaytracingAccelerationStructure SceneTlas;
Texture2D GBufferBaseColor;
Texture2D GBufferNormal;
Texture2D GBufferMaterial;
Texture2D GBufferDeviceZ;

static const uint RTIndirectSpecularRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
static const uint RTIndirectSpecularInstanceMask = 0xFFu;

bool IsSkyPixel(float deviceZ)
{
	return deviceZ <= 1.0e-6f;
}

float3 DecodeNormal(float3 normalWorld)
{
	const float lengthSquared = dot(normalWorld, normalWorld);
	return lengthSquared > 0.0f ? normalWorld * rsqrt(lengthSquared) : float3(0.0f, 1.0f, 0.0f);
}

float TraceStageOneSignal(float3 originWorld, float3 normalWorld)
{
	RayDesc ray;
	ray.Origin = originWorld + normalWorld * 0.01f;
	ray.Direction = normalWorld;
	ray.TMin = 0.001f;
	ray.TMax = 0.01f;

	RayQuery<RTIndirectSpecularRayFlags> query;
	query.TraceRayInline(SceneTlas, RTIndirectSpecularRayFlags, RTIndirectSpecularInstanceMask, ray);
	while (query.Proceed())
	{
	}

	return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 1.0f : 0.0f;
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
	const float deviceZ = GBufferDeviceZ.Load(pixel).r;
	if (IsSkyPixel(deviceZ))
	{
		IndirectSpecularTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		return;
	}

	const float3 baseColor = GBufferBaseColor.Load(pixel).rgb;
	const float3 normalWorld = DecodeNormal(GBufferNormal.Load(pixel).xyz);
	const float2 materialData = GBufferMaterial.Load(pixel).rg;
	const float3 originWorld = Camera.Position + normalWorld * (0.001f + 0.0f * (baseColor.r + materialData.x + materialData.y));
	const float hitSignal = TraceStageOneSignal(originWorld, normalWorld);
	const float bindingKeepAliveSignal =
	    float(FrameIndex & 1u) * 1.0e-6f +
	    saturate(baseColor.r + materialData.x + materialData.y) * 1.0e-9f;

	IndirectSpecularTexture[dispatchThreadId.xy] = float4(0.0f, 0.0f, 0.0f, hitSignal + bindingKeepAliveSignal);
}
