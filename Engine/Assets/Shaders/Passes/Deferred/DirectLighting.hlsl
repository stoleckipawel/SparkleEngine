#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/DirectLightingCommon.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> DirectDiffuseTexture;
RWTexture2D<float4> DirectSpecularTexture;
RWTexture2D<float4> DirectSubsurfaceTexture;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	DirectDiffuseTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const GBufferData gBuffer = LoadGBuffer(dispatchThreadId.xy);
	if (IsSkyPixel(gBuffer.DeviceZ))
	{
		DirectDiffuseTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		DirectSpecularTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		DirectSubsurfaceTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		return;
	}

	const float3 positionWorld =
	    ReconstructGBufferWorldPosition(dispatchThreadId.xy, gBuffer.DeviceZ, Camera.InvViewMTX, Camera.InvProjectionMTX);
	const float3 viewDirWorld = normalize(Camera.Position - positionWorld);

	float3 directDiffuse = 0.0f;
	float3 directSpecular = 0.0f;
	float3 directSubsurface = 0.0f;
	const bool evaluateSubsurface = any(gBuffer.SubsurfaceColor > 0.0f.xxx) && gBuffer.SubsurfaceStrength > 0.0f;
	const uint directionalLightCount = min(ViewLighting.DirectionalLightCount, MAX_DIRECTIONAL_LIGHTS);
	const uint pointLightCount = min(ViewLighting.PointLightCount, MAX_POINT_LIGHTS);
	const uint spotLightCount = min(ViewLighting.SpotLightCount, MAX_SPOT_LIGHTS);

	[loop] for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		float3 lightDiffuse;
		float3 lightSpecular;
		float3 lightSubsurface;
		DirectLighting::AccumulateDirectionalLight(
		    viewDirWorld,
		    gBuffer.NormalWorld,
		    gBuffer.Roughness,
		    evaluateSubsurface,
		    lightIndex,
		    lightDiffuse,
		    lightSpecular,
		    lightSubsurface);

		directDiffuse += lightDiffuse;
		directSpecular += lightSpecular;
		directSubsurface += lightSubsurface;
	}

	[loop] for (uint lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
	{
		float3 lightDiffuse;
		float3 lightSpecular;
		float3 lightSubsurface;
		DirectLighting::AccumulatePointLight(
		    positionWorld,
		    viewDirWorld,
		    gBuffer.NormalWorld,
		    gBuffer.Roughness,
		    evaluateSubsurface,
		    lightIndex,
		    lightDiffuse,
		    lightSpecular,
		    lightSubsurface);

		directDiffuse += lightDiffuse;
		directSpecular += lightSpecular;
		directSubsurface += lightSubsurface;
	}

	[loop] for (uint lightIndex = 0; lightIndex < spotLightCount; ++lightIndex)
	{
		float3 lightDiffuse;
		float3 lightSpecular;
		float3 lightSubsurface;
		DirectLighting::AccumulateSpotLight(
		    positionWorld,
		    viewDirWorld,
		    gBuffer.NormalWorld,
		    gBuffer.Roughness,
		    evaluateSubsurface,
		    lightIndex,
		    lightDiffuse,
		    lightSpecular,
		    lightSubsurface);

		directDiffuse += lightDiffuse;
		directSpecular += lightSpecular;
		directSubsurface += lightSubsurface;
	}

	DirectDiffuseTexture[dispatchThreadId.xy] = float4(directDiffuse, gBuffer.Alpha);
	DirectSpecularTexture[dispatchThreadId.xy] = float4(directSpecular, gBuffer.Alpha);
	DirectSubsurfaceTexture[dispatchThreadId.xy] = float4(directSubsurface, gBuffer.Alpha);
}
