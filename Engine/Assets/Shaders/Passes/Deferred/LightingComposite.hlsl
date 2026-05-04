#include "Resources/ConstantBuffers.hlsli"
#include "BRDF/BRDF.hlsli"
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

struct SurfaceResponse
{
	float3 DiffuseColor;
	float3 DiffuseWeight;
	float3 Fresnel;
	float3 SubsurfaceColor;
	float IndirectSpecularOcclusion;
	float3 IndirectDiffuseOcclusion;
	float IndirectSubsurfaceOcclusion;
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

SurfaceResponse EvaluateSurfaceResponse(GBufferData gBuffer, float3 viewDirWorld)
{
	SurfaceResponse response;
	const float NoV = saturate(dot(gBuffer.NormalWorld, viewDirWorld));
	const float3 dielectricF0 = float3(0.04f, 0.04f, 0.04f);
	const float3 f0 = lerp(dielectricF0, gBuffer.BaseColor, gBuffer.Metallic);

	response.DiffuseColor = gBuffer.BaseColor;
	response.Fresnel = BRDF::Fresnel::EvaluateIndirect(NoV, f0, gBuffer.Roughness);
	response.DiffuseWeight = (1.0f.xxx - response.Fresnel) * (1.0f - gBuffer.Metallic);
	response.SubsurfaceColor = (1.0f - gBuffer.Metallic) * gBuffer.SubsurfaceColor * gBuffer.SubsurfaceStrength;
	response.IndirectDiffuseOcclusion = BRDF::Occlusion::MultibounceAO(gBuffer.AmbientOcclusion, gBuffer.BaseColor);
	response.IndirectSpecularOcclusion = BRDF::Occlusion::SpecularOcclusion(NoV, gBuffer.AmbientOcclusion, gBuffer.Roughness);
	response.IndirectSubsurfaceOcclusion = gBuffer.AmbientOcclusion;
	return response;
}

float3 ComposeDiffuseLighting(LightingTerms terms, SurfaceResponse response)
{
	const float3 directDiffuse = terms.DirectDiffuse * response.DiffuseWeight * response.DiffuseColor;
	const float3 indirectDiffuse =
	    terms.IndirectDiffuse * response.DiffuseWeight * response.DiffuseColor * response.IndirectDiffuseOcclusion;
	return directDiffuse + indirectDiffuse;
}

float3 ComposeSpecularLighting(LightingTerms terms, SurfaceResponse response)
{
	const float3 directSpecular = terms.DirectSpecular * response.Fresnel;
	const float3 indirectSpecular =
	    terms.IndirectSpecular * response.Fresnel * response.IndirectSpecularOcclusion;
	return directSpecular + indirectSpecular;
}

float3 ComposeSubsurfaceLighting(LightingTerms terms, SurfaceResponse response)
{
	const float3 directSubsurface = terms.DirectSubsurface * response.SubsurfaceColor;
	const float3 indirectSubsurface =
	    terms.IndirectSubsurface * response.SubsurfaceColor * response.IndirectSubsurfaceOcclusion;
	return directSubsurface + indirectSubsurface;
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
	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, gBuffer.DeviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 viewDirWorld = normalize(Camera.Position - positionWorld);
	const SurfaceResponse surface = EvaluateSurfaceResponse(gBuffer, viewDirWorld);

	const float3 diffuseLighting = ComposeDiffuseLighting(lighting, surface);
	const float3 specularLighting = ComposeSpecularLighting(lighting, surface);
	const float3 subsurfaceLighting = ComposeSubsurfaceLighting(lighting, surface);
	const float3 lit = diffuseLighting + specularLighting + subsurfaceLighting + gBuffer.Emissive;
	SceneColorTexture[dispatchThreadId.xy] = float4(lit, gBuffer.Alpha);
}