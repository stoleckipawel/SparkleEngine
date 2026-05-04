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
	if (gBuffer.DeviceZ >= 0.999999f)
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
	const uint directionalLightCount = min(ViewLighting.DirectionalLightCount, MAX_DIRECTIONAL_LIGHTS);

	[loop] for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		float3 lightDiffuse;
		float3 lightSpecular;
		float3 lightSubsurface;
		DirectLighting::AccumulateDirectionalLight(
		    viewDirWorld,
		    gBuffer.NormalWorld,
		    gBuffer.Roughness,
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