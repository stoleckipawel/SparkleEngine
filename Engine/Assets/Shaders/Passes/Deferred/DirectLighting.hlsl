#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/DirectLightingCommon.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"
#if defined(SPARKLE_DIRECT_LIGHTING_NO_RAY_QUERY)
#include "Passes/Deferred/RayTracedShadowSignals.hlsli"
#else
#include "Passes/Deferred/RayTracedShadows.hlsli"
#endif

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
	const uint directionalLightCount = ViewLighting.DirectionalLightCount;
	const uint pointLightCount = ViewLighting.PointLightCount;
	const uint spotLightCount = ViewLighting.SpotLightCount;

	[loop] for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		const float3 lightDirection = DirectLighting::GetDirectionalLightDirection(lightIndex);
#if defined(SPARKLE_DIRECT_LIGHTING_NO_RAY_QUERY)
		const ShadowVisibilitySignal directionalShadow = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
#else
		const ShadowVisibilitySignal directionalShadow = RayTracedShadows::TraceDirectionalShadowSignal(
		    positionWorld,
		    gBuffer.NormalWorld,
		    lightDirection,
		    DirectionalLights[lightIndex].AngularDiameter,
		    dispatchThreadId.xy,
		    lightIndex,
		    DirectionalLights[lightIndex].CastShadow != 0u);
#endif
		float3 lightDiffuse;
		float3 lightSpecular;
		float3 lightSubsurface;
		DirectLighting::AccumulateDirectionalLight(
		    viewDirWorld,
		    gBuffer.NormalWorld,
		    gBuffer.Roughness,
		    evaluateSubsurface,
		    lightIndex,
		    directionalShadow.Visibility,
		    lightDiffuse,
		    lightSpecular,
		    lightSubsurface);

		directDiffuse += lightDiffuse;
		directSpecular += lightSpecular;
		directSubsurface += lightSubsurface;
	}

	[loop] for (uint lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
	{
#if defined(SPARKLE_DIRECT_LIGHTING_NO_RAY_QUERY)
		const ShadowVisibilitySignal pointShadow = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
#else
		const ShadowVisibilitySignal pointShadow = RayTracedShadows::TracePointShadowSignal(
		    positionWorld,
		    gBuffer.NormalWorld,
		    PointLights[lightIndex].Position,
		    PointLights[lightIndex].Range,
		    PointLights[lightIndex].SourceRadius,
		    dispatchThreadId.xy,
		    lightIndex,
		    PointLights[lightIndex].CastShadow != 0u);
#endif
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
		    pointShadow.Visibility,
		    lightDiffuse,
		    lightSpecular,
		    lightSubsurface);

		directDiffuse += lightDiffuse;
		directSpecular += lightSpecular;
		directSubsurface += lightSubsurface;
	}

	[loop] for (uint lightIndex = 0; lightIndex < spotLightCount; ++lightIndex)
	{
#if defined(SPARKLE_DIRECT_LIGHTING_NO_RAY_QUERY)
		const ShadowVisibilitySignal spotShadow = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
#else
		const ShadowVisibilitySignal spotShadow = RayTracedShadows::TraceSpotShadowSignal(
		    positionWorld,
		    gBuffer.NormalWorld,
		    SpotLights[lightIndex].Position,
		    SpotLights[lightIndex].Direction,
		    SpotLights[lightIndex].Range,
		    SpotLights[lightIndex].SourceRadius,
		    SpotLights[lightIndex].OuterConeCosine,
		    dispatchThreadId.xy,
		    lightIndex,
		    SpotLights[lightIndex].CastShadow != 0u);
#endif
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
		    spotShadow.Visibility,
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
