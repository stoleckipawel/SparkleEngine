#ifndef SPARKLE_GEOMETRY_FULLSCREEN_TRIANGLE_HLSLI
#define SPARKLE_GEOMETRY_FULLSCREEN_TRIANGLE_HLSLI

struct FullscreenVertexOutput
{
	float4 Position : SV_Position;
	float2 Uv : TEXCOORD0;
};

FullscreenVertexOutput BuildFullscreenTriangleVertex(uint vertexId)
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

#endif
