#include "/Engine/Resources/ViewUniformData.hlsli"

#include "/Engine/Resources/RenderViewModeConstants.hlsli"
#include "/Engine/Passes/GBuffer/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColor;
Texture2D DirectDiffuse;
Texture2D DirectSpecular;
Texture2D DirectSubsurface;
Texture2D IndirectDiffuse;
Texture2D IndirectSpecular;

float3 PreviewScalar(float value)
{
	return saturate(value).xxx;
}

float3 PreviewNormal(float3 normalWorld)
{
	return normalize(normalWorld) * 0.5f + 0.5f;
}

float3 PreviewHdr(float3 color)
{
	const float3 safeColor = max(color, 0.0f);
	return safeColor / (1.0f + safeColor);
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (RenderViewModeIndex < RenderViewMode::GBufferDiffuse)
	{
		return;
	}

	uint width = 0;
	uint height = 0;
	SceneColor.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const int3 pixel = int3(dispatchThreadId.xy, 0);
	const GBufferData gBuffer = LoadGBuffer(dispatchThreadId.xy);
	const float3 directDiffuse = DirectDiffuse.Load(pixel).rgb;
	const float3 directSpecular = DirectSpecular.Load(pixel).rgb;
	const float3 directSubsurface = DirectSubsurface.Load(pixel).rgb;
	const float3 indirectDiffuse = IndirectDiffuse.Load(pixel).rgb;
	const float3 indirectSpecular = IndirectSpecular.Load(pixel).rgb;

	float3 outputColor = 0.0f;
	switch (RenderViewModeIndex)
	{
		case RenderViewMode::GBufferDiffuse:
			outputColor = saturate(gBuffer.BaseColor);
			break;
		case RenderViewMode::GBufferNormal:
			outputColor = PreviewNormal(gBuffer.NormalWorld);
			break;
		case RenderViewMode::GBufferRoughness:
			outputColor = PreviewScalar(gBuffer.Roughness);
			break;
		case RenderViewMode::GBufferMetallic:
			outputColor = PreviewScalar(gBuffer.Metallic);
			break;
		case RenderViewMode::GBufferEmissive:
			outputColor = PreviewHdr(gBuffer.Emissive);
			break;
		case RenderViewMode::GBufferAmbientOcclusion:
			outputColor = PreviewScalar(gBuffer.AmbientOcclusion);
			break;
		case RenderViewMode::GBufferSubsurfaceColor:
			outputColor = saturate(gBuffer.SubsurfaceColor);
			break;
		case RenderViewMode::GBufferSubsurfaceStrength:
			outputColor = PreviewScalar(gBuffer.SubsurfaceStrength);
			break;
		case RenderViewMode::DirectDiffuse:
			outputColor = PreviewHdr(directDiffuse);
			break;
		case RenderViewMode::DirectSpecular:
			outputColor = PreviewHdr(directSpecular);
			break;
		case RenderViewMode::DirectSubsurface:
			outputColor = PreviewHdr(directSubsurface);
			break;
		case RenderViewMode::IndirectDiffuse:
			outputColor = PreviewHdr(indirectDiffuse);
			break;
		case RenderViewMode::IndirectSpecular:
			outputColor = PreviewHdr(indirectSpecular);
			break;
		case RenderViewMode::GpuSceneInstances:
			outputColor = saturate(gBuffer.BaseColor);
			break;
		default:
			return;
	}

	SceneColor[dispatchThreadId.xy] = float4(outputColor, gBuffer.Alpha);
}
