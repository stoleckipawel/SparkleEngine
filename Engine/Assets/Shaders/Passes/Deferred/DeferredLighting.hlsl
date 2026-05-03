#include "Resources/ConstantBuffers.hlsli"
#include "BRDF/BRDF.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColorTexture;

void AccumulateDirectionalLight(
	float3 viewDirWorld,
	float3 normalWorld,
	float3 baseColor,
	float roughness,
	float metallic,
	uint lightIndex,
	out float3 outDiffuse,
	out float3 outSpecular)
{
	const float3 lightDirection = normalize(-ViewLighting.DirectionalLights[lightIndex].Direction);
	BRDF::ShadingData shadingData = BRDF::ComputeShadingData(normalWorld, viewDirWorld, lightDirection);

	if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f)
	{
		outDiffuse = 0.0f;
		outSpecular = 0.0f;
		return;
	}

	const float3 radiance = ViewLighting.DirectionalLights[lightIndex].Color * ViewLighting.DirectionalLights[lightIndex].Intensity;
	const float3 dielectricF0 = float3(0.04f, 0.04f, 0.04f);
	const float3 f0 = lerp(dielectricF0, baseColor, saturate(metallic));

	float3 directSubsurface;
	BRDF::Direct::Evaluate(
	    shadingData,
	    baseColor,
	    max(roughness, 0.04f),
	    saturate(metallic),
	    f0,
	    float3(0.0f, 0.0f, 0.0f),
	    0.0f,
	    outDiffuse,
	    outSpecular,
	    directSubsurface);

	outDiffuse *= radiance * shadingData.NoL;
	outSpecular *= radiance * shadingData.NoL;
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

	const GBufferData gBuffer = LoadGBuffer(dispatchThreadId.xy);
	const float3 viewDirWorld = normalize(-Camera.Direction);

	float3 directDiffuse = 0.0f;
	float3 directSpecular = 0.0f;
	const uint directionalLightCount = min(ViewLighting.DirectionalLightCount, MAX_DIRECTIONAL_LIGHTS);

	[loop] for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		float3 lightDiffuse;
		float3 lightSpecular;
		AccumulateDirectionalLight(
		    viewDirWorld,
		    gBuffer.NormalWorld,
		    gBuffer.BaseColor,
		    gBuffer.Roughness,
		    gBuffer.Metallic,
		    lightIndex,
		    lightDiffuse,
		    lightSpecular);

		directDiffuse += lightDiffuse;
		directSpecular += lightSpecular;
	}

	const float3 lit = directDiffuse * gBuffer.AmbientOcclusion + directSpecular + gBuffer.Emissive;
	SceneColorTexture[dispatchThreadId.xy] = float4(lit, gBuffer.Alpha);
}