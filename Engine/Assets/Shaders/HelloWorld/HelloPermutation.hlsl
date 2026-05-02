struct VSInput
{
	float3 Position : POSITION;
	float3 Color : COLOR0;
};

struct VSOutput
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR0;
};

VSOutput VSMain(VSInput Input)
{
	VSOutput Output;
	Output.Position = float4(Input.Position, 1.0f);
	Output.Color = Input.Color;
	return Output;
}

float4 PSMain(VSOutput Input) : SV_Target0
{
	float3 Color = Input.Color;

#if HELLO_PERMUTATION_SHADE_MODE == 1
	Color = lerp(Color, float3(1.0f, 0.68f, 0.32f), 0.45f);
#elif HELLO_PERMUTATION_SHADE_MODE == 2
	Color = lerp(Color, float3(0.20f, 0.64f, 1.0f), 0.45f);
#endif

#if HELLO_PERMUTATION_USE_TINT
	Color *= float3(0.72f, 1.0f, 0.86f);
#endif

	return float4(Color, 1.0f);
}