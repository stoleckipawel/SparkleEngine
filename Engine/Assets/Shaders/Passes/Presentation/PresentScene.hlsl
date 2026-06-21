struct FullscreenVertexOutput
{
	float4 Position : SV_Position;
	float2 Uv : TEXCOORD0;
};

Texture2D SceneColor;
SamplerState SamplerLinearClamp;

FullscreenVertexOutput VSMain(uint vertexId : SV_VertexID)
{
	const float2 positions[3] = {
	    float2(-1.0f, -1.0f),
	    float2(-1.0f, 3.0f),
	    float2(3.0f, -1.0f),
	};

	FullscreenVertexOutput output;
	output.Position = float4(positions[vertexId], 0.0f, 1.0f);
	output.Uv = float2(0.5f * output.Position.x + 0.5f, 0.5f - 0.5f * output.Position.y);
	return output;
}

float3 ToneMapReinhard(float3 color)
{
	const float3 positiveColor = max(color, 0.0f.xxx);
	return positiveColor / (positiveColor + 1.0f.xxx);
}

float4 PSMain(FullscreenVertexOutput input) : SV_Target0
{
	const float4 sceneColor = SceneColor.SampleLevel(SamplerLinearClamp, input.Uv, 0.0f);
	return float4(ToneMapReinhard(sceneColor.rgb), saturate(sceneColor.a));
}
