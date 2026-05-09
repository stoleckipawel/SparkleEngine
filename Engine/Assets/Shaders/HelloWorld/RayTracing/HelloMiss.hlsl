struct HelloRayPayload
{
	float3 Color;
};

[shader("miss")] void HelloMiss(inout HelloRayPayload Payload)
{
	Payload.Color = float3(0.08f, 0.12f, 0.20f);
}
