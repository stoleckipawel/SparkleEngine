struct HelloRayPayload
{
	float3 Color;
};

RaytracingAccelerationStructure SceneAccelerationStructure;
RWTexture2D<float4> OutputTexture;

[shader("raygeneration")] void HelloRayGen()
{
	uint2 Pixel = DispatchRaysIndex().xy;
	uint2 Extent = DispatchRaysDimensions().xy;
	float2 Uv = (float2(Pixel) + 0.5f) / max(float2(Extent), float2(1.0f, 1.0f));

	RayDesc Ray;
	Ray.Origin = float3(0.0f, 0.0f, -2.0f);
	Ray.Direction = normalize(float3(Uv * 2.0f - 1.0f, 1.0f));
	Ray.TMin = 0.001f;
	Ray.TMax = 1000.0f;

	HelloRayPayload Payload;
	Payload.Color = float3(0.02f, 0.04f, 0.08f);
	TraceRay(SceneAccelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 1, 0, Ray, Payload);
	OutputTexture[Pixel] = float4(Payload.Color, 1.0f);
}
