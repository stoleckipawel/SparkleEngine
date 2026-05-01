struct HelloRayPayload
{
	float3 Color;
};

struct HelloAttributes
{
	float2 Barycentrics;
};

[shader("anyhit")]
void HelloAnyHit(inout HelloRayPayload Payload, in HelloAttributes Attributes)
{
	if (Attributes.Barycentrics.x + Attributes.Barycentrics.y < 0.05f)
	{
		IgnoreHit();
	}
}
