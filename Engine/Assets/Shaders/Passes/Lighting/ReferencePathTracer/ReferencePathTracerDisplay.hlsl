#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerUniformData.hlsli"
#include "/Engine/RayTracing/PathAccumulation.hlsli"

Texture2D<float4> WorkingMean;
Texture2D<float4> WorkingM2;
RWTexture2D<float4> CommittedMean;
RWTexture2D<float4> CommittedM2;
RWTexture2D<float4> SceneColor;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width;
	uint height;
	SceneColor.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	float4 display = 0.0f.xxxx;
	if ((WorkFlags & ReferencePathTracerWorkFlag_ClearDisplay) == 0u)
	{
		display = CommittedMean[pixelCoord];
	}
	if ((WorkFlags & ReferencePathTracerWorkFlag_CommitPrefix) != 0u)
	{
		display = WorkingMean[pixelCoord];
		const float3 m2 = WorkingM2[pixelCoord].rgb;
		const float3 standardError = PathAccumulation::StandardError(m2, PriorSampleCount + 1u);
		CommittedM2[pixelCoord] = float4(m2, max(standardError.x, max(standardError.y, standardError.z)));
	}
	CommittedMean[pixelCoord] = display;
	SceneColor[pixelCoord] = display;
}
