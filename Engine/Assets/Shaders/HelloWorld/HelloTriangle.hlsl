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
	return float4(Input.Color, 1.0f);
}