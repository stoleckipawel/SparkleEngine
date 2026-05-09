RaytracingAccelerationStructure SceneAccelerationStructure;
RWTexture2D<float4> OutputTexture;

[numthreads(8, 8, 1)] void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	RayDesc Ray;
	Ray.Origin = float3(0.0f, 0.0f, -2.0f);
	Ray.Direction = normalize(float3((float2(DispatchThreadId.xy) / 64.0f) * 2.0f - 1.0f, 1.0f));
	Ray.TMin = 0.001f;
	Ray.TMax = 1000.0f;

	RayQuery<RAY_FLAG_NONE> Query;
	Query.TraceRayInline(SceneAccelerationStructure, RAY_FLAG_NONE, 0xFF, Ray);
	while (Query.Proceed())
	{
	}

	const bool Hit = Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
	OutputTexture[DispatchThreadId.xy] = Hit ? float4(0.95f, 0.74f, 0.22f, 1.0f) : float4(0.05f, 0.08f, 0.12f, 1.0f);
}
