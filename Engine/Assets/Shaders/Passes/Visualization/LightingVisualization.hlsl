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
	uint width, height;
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
			outputColor = DirectDiffuse.Load(pixel).rgb;
			break;
		case RenderViewMode::DirectSpecular:
			outputColor = DirectSpecular.Load(pixel).rgb;
			break;
		case RenderViewMode::DirectSubsurface:
			outputColor = DirectSubsurface.Load(pixel).rgb;
			break;
		case RenderViewMode::IndirectDiffuse:
			outputColor = IndirectDiffuse.Load(pixel).rgb;
			break;
		case RenderViewMode::IndirectSpecular:
			outputColor = IndirectSpecular.Load(pixel).rgb;
			break;
	}
	SceneColor[dispatchThreadId.xy] = float4(max(outputColor, 0.0f), GBufferBaseColor.Load(pixel).a);
}
