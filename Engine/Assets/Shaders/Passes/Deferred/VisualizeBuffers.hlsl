#include "Resources/ConstantBuffers.hlsli"
#include "Debug/RenderViewModeConstants.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColorTexture;
Texture2D DirectDiffuse;
Texture2D DirectSpecular;
Texture2D DirectSubsurface;
Texture2D IndirectDiffuse;
Texture2D IndirectSpecular;
Texture2D IndirectSubsurface;

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
	if (ViewModeIndex == ViewMode::Lit)
	{
		return;
	}

	uint width = 0;
	uint height = 0;
	SceneColorTexture.GetDimensions(width, height);

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
	const float3 indirectSubsurface = IndirectSubsurface.Load(pixel).rgb;

	float3 outputColor = 0.0f;
	switch (ViewModeIndex)
	{
		case ViewMode::GBufferDiffuse:
			outputColor = saturate(gBuffer.BaseColor);
			break;
		case ViewMode::GBufferNormal:
			outputColor = PreviewNormal(gBuffer.NormalWorld);
			break;
		case ViewMode::GBufferRoughness:
			outputColor = PreviewScalar(gBuffer.Roughness);
			break;
		case ViewMode::GBufferMetallic:
			outputColor = PreviewScalar(gBuffer.Metallic);
			break;
		case ViewMode::GBufferEmissive:
			outputColor = PreviewHdr(gBuffer.Emissive);
			break;
		case ViewMode::GBufferAmbientOcclusion:
			outputColor = PreviewScalar(gBuffer.AmbientOcclusion);
			break;
		case ViewMode::GBufferSubsurfaceColor:
			outputColor = saturate(gBuffer.SubsurfaceColor);
			break;
		case ViewMode::GBufferSubsurfaceStrength:
			outputColor = PreviewScalar(gBuffer.SubsurfaceStrength);
			break;
		case ViewMode::DirectDiffuse:
			outputColor = PreviewHdr(directDiffuse);
			break;
		case ViewMode::DirectSpecular:
			outputColor = PreviewHdr(directSpecular);
			break;
		case ViewMode::DirectSubsurface:
			outputColor = PreviewHdr(directSubsurface);
			break;
		case ViewMode::IndirectDiffuse:
			outputColor = PreviewHdr(indirectDiffuse);
			break;
		case ViewMode::IndirectSpecular:
			outputColor = PreviewHdr(indirectSpecular);
			break;
		case ViewMode::IndirectSubsurface:
			outputColor = PreviewHdr(indirectSubsurface);
			break;
		case ViewMode::InstanceGroups:
			outputColor = saturate(gBuffer.BaseColor);
			break;
		default:
			return;
	}

	SceneColorTexture[dispatchThreadId.xy] = float4(outputColor, gBuffer.Alpha);
}