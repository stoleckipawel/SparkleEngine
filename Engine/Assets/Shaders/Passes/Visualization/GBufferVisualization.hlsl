#include "/Engine/Resources/ViewUniformData.hlsli"

#include "/Engine/Passes/Visualization/VisualizationPreview.hlsli"
#include "/Engine/Resources/RenderViewModeConstants.hlsli"

RWTexture2D<float4> SceneColor;
Texture2D GBufferBaseColor;
Texture2D GBufferNormal;
Texture2D GBufferMaterial;
Texture2D GBufferEmissive;
Texture2D GBufferSubsurface;

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
	const float4 baseColor = GBufferBaseColor.Load(pixel);
	float3 outputColor = 0.0f;

	switch (RenderViewModeIndex)
	{
		case RenderViewMode::GBufferDiffuse:
			outputColor = saturate(baseColor.rgb);
			break;
		case RenderViewMode::GBufferNormal:
			outputColor = VisualizeNormal(GBufferNormal.Load(pixel).xyz);
			break;
		case RenderViewMode::GBufferRoughness:
			outputColor = VisualizeScalar(GBufferMaterial.Load(pixel).g);
			break;
		case RenderViewMode::GBufferMetallic:
			outputColor = VisualizeScalar(GBufferMaterial.Load(pixel).r);
			break;
		case RenderViewMode::GBufferEmissive:
			outputColor = max(GBufferEmissive.Load(pixel).rgb, 0.0f);
			break;
		case RenderViewMode::GBufferAmbientOcclusion:
			outputColor = VisualizeScalar(GBufferMaterial.Load(pixel).b);
			break;
		case RenderViewMode::GBufferSubsurfaceColor:
			outputColor = saturate(GBufferSubsurface.Load(pixel).rgb);
			break;
		case RenderViewMode::GBufferSubsurfaceStrength:
			outputColor = VisualizeScalar(GBufferSubsurface.Load(pixel).a);
			break;
		default:
			return;
	}

	SceneColor[dispatchThreadId.xy] = float4(outputColor, baseColor.a);
}
