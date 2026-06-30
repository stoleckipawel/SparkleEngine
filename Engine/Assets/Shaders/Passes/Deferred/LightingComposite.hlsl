#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColorTexture;
Texture2D DirectDiffuse;
Texture2D DirectSpecular;
Texture2D DirectSubsurface;
Texture2D IndirectDiffuse;
Texture2D IndirectSpecular;
Texture2D IndirectSubsurface;

struct LightingTerms
{
	float3 DirectDiffuse;
	float3 DirectSpecular;
	float3 DirectSubsurface;
	float3 IndirectDiffuse;
	float3 IndirectSpecular;
	float3 IndirectSubsurface;
};

LightingTerms LoadLightingTerms(int3 pixel)
{
	LightingTerms terms;
	terms.DirectDiffuse = DirectDiffuse.Load(pixel).rgb;
	terms.DirectSpecular = DirectSpecular.Load(pixel).rgb;
	terms.DirectSubsurface = DirectSubsurface.Load(pixel).rgb;
	terms.IndirectDiffuse = IndirectDiffuse.Load(pixel).rgb;
	terms.IndirectSpecular = IndirectSpecular.Load(pixel).rgb;
	terms.IndirectSubsurface = IndirectSubsurface.Load(pixel).rgb;
	return terms;
}

float3 ComposeDiffuseLighting(LightingTerms terms)
{
	return terms.DirectDiffuse + terms.IndirectDiffuse;
}

float3 ComposeSpecularLighting(LightingTerms terms)
{
	return terms.DirectSpecular + terms.IndirectSpecular;
}

float3 ComposeSubsurfaceLighting(LightingTerms terms)
{
	return terms.DirectSubsurface + terms.IndirectSubsurface;
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	SceneColorTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const int3 pixel = int3(dispatchThreadId.xy, 0);
	const GBufferData gBuffer = LoadGBuffer(dispatchThreadId.xy);
	const LightingTerms lighting = LoadLightingTerms(pixel);
	const float3 diffuseLighting = ComposeDiffuseLighting(lighting);
	const float3 specularLighting = ComposeSpecularLighting(lighting);
	const float3 subsurfaceLighting = ComposeSubsurfaceLighting(lighting);
	// Demodulated irradiance/albedo signals belong in separate buffers, never in this sum.
	const float3 lit = diffuseLighting + specularLighting + subsurfaceLighting + gBuffer.Emissive;
	SceneColorTexture[dispatchThreadId.xy] = float4(lit, gBuffer.Alpha);
}
