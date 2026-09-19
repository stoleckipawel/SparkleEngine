#include "/Engine/Resources/ViewUniformData.hlsli"

#include "/Engine/Resources/RenderViewModeConstants.hlsli"

RWTexture2D<float4> SceneColor;
Texture2D GBufferBaseColor;
Texture2D DirectDiffuse;
Texture2D DirectSpecular;
Texture2D DirectSubsurface;
Texture2D IndirectDiffuse;
Texture2D IndirectSpecular;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	SceneColor.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const int3 pixel = int3(dispatchThreadId.xy, 0);
	float3 outputColor = 0.0f;

	switch (RenderViewModeIndex)
	{
		case RenderViewMode::DirectDiffuse:
			outputColor = max(DirectDiffuse.Load(pixel).rgb, 0.0f);
			break;
		case RenderViewMode::DirectSpecular:
			outputColor = max(DirectSpecular.Load(pixel).rgb, 0.0f);
			break;
		case RenderViewMode::DirectSubsurface:
			outputColor = max(DirectSubsurface.Load(pixel).rgb, 0.0f);
			break;
		case RenderViewMode::IndirectDiffuse:
			outputColor = max(IndirectDiffuse.Load(pixel).rgb, 0.0f);
			break;
		case RenderViewMode::IndirectSpecular:
			outputColor = max(IndirectSpecular.Load(pixel).rgb, 0.0f);
			break;
		default:
			return;
	}

	SceneColor[dispatchThreadId.xy] = float4(outputColor, GBufferBaseColor.Load(pixel).a);
}
