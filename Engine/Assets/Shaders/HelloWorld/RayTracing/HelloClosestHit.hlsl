struct HelloRayPayload
{
	float3 Color;
};

struct HelloAttributes
{
	float2 Barycentrics;
};

[shader("closesthit")]
void HelloClosestHit(inout HelloRayPayload Payload, in HelloAttributes Attributes)
{
	Payload.Color = float3(0.95f, 0.74f, 0.22f) * (0.5f + 0.5f * Attributes.Barycentrics.x);
}
