struct HelloCallableData
{
	float3 Color;
};

[shader("callable")] void HelloCallable(inout HelloCallableData Data)
{
	Data.Color = float3(0.30f, 0.65f, 1.0f);
}
