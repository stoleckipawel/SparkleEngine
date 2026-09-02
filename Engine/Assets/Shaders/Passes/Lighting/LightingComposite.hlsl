RWTexture2D<float4> SceneColor;
Texture2D DirectDiffuse;
Texture2D DirectSpecular;
Texture2D DirectSubsurface;
Texture2D IndirectDiffuse;
Texture2D IndirectSpecular;
Texture2D GBufferBaseColor;
Texture2D GBufferEmissive;

struct LightingTerms
{
	float3 DirectDiffuse;
	float3 DirectSpecular;
	float3 DirectSubsurface;
	float3 IndirectDiffuse;
	float3 IndirectSpecular;
};

LightingTerms LoadLightingTerms(int3 pixel)
{
	LightingTerms terms;
	terms.DirectDiffuse = DirectDiffuse.Load(pixel).rgb;
	terms.DirectSpecular = DirectSpecular.Load(pixel).rgb;
	terms.DirectSubsurface = DirectSubsurface.Load(pixel).rgb;
	terms.IndirectDiffuse = IndirectDiffuse.Load(pixel).rgb;
	terms.IndirectSpecular = IndirectSpecular.Load(pixel).rgb;
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
	const LightingTerms lighting = LoadLightingTerms(pixel);
	const float3 diffuseLighting = ComposeDiffuseLighting(lighting);
	const float3 specularLighting = ComposeSpecularLighting(lighting);
	const float3 emissive = max(GBufferEmissive.Load(pixel).rgb, 0.0f);
	const float alpha = GBufferBaseColor.Load(pixel).a;
	const float3 lit = diffuseLighting + specularLighting + lighting.DirectSubsurface + emissive;
	SceneColor[dispatchThreadId.xy] = float4(lit, alpha);
}
