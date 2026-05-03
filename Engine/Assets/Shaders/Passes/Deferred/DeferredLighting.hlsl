#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/DeferredDirectLighting.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColorTexture;

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
	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, gBuffer.DeviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 viewDirWorld = normalize(Camera.Position - positionWorld);

	float3 directDiffuse = 0.0f;
	float3 directSpecular = 0.0f;
	const uint directionalLightCount = min(ViewLighting.DirectionalLightCount, MAX_DIRECTIONAL_LIGHTS);

	[loop] for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		float3 lightDiffuse;
		float3 lightSpecular;
		DeferredDirectLighting::AccumulateDirectionalLight(
		    positionWorld,
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