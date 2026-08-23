#include "/Engine/Resources/ViewCameraUniformData.hlsli"

#include "/Engine/Lighting/DirectLightReservoir.hlsli"
#include "/Engine/Lighting/SurfaceLighting.hlsli"
#include "/Engine/Passes/GBuffer/GBufferUtils.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowSignalPacking.hlsli"
RWTexture2D<float4> DirectDiffuse;
RWTexture2D<float4> DirectSpecular;
RWTexture2D<float4> DirectSubsurface;
Texture2D<float4> ShadowVisibilitySignal;
Texture2D<float4> CurrentReservoirSample;
Texture2D<float4> CurrentReservoirWeight;

void AddDirectLightSample(GBufferData gBuffer,
                          float3 viewDirWorld,
                          bool evaluateSubsurface,
                          LightSampling::DirectLightSample lightSample,
                          ShadowVisibilitySignal shadow,
                          float sampleWeight,
                          inout float3 directDiffuse,
                          inout float3 directSpecular,
                          inout float3 directSubsurface)
{
	if (!lightSample.Valid || sampleWeight <= 0.0f)
	{
		return;
	}

	float3 lightDiffuse;
	float3 lightSpecular;
	float3 lightSubsurface;
	SurfaceLighting::AccumulateDirectLightSample(viewDirWorld,
	                                             gBuffer.NormalWorld,
	                                             gBuffer.BaseColor,
	                                             gBuffer.Roughness,
	                                             gBuffer.Metallic,
	                                             gBuffer.DielectricF0,
	                                             gBuffer.SubsurfaceColor,
	                                             gBuffer.SubsurfaceStrength,
	                                             evaluateSubsurface,
	                                             lightSample,
	                                             shadow.Visibility,
	                                             lightDiffuse,
	                                             lightSpecular,
	                                             lightSubsurface);

	directDiffuse += lightDiffuse * sampleWeight;
	directSpecular += lightSpecular * sampleWeight;
	directSubsurface += lightSubsurface * sampleWeight;
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	DirectDiffuse.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const GBufferData gBuffer = LoadGBuffer(dispatchThreadId.xy);
	if (IsSkyPixel(gBuffer.SceneDepth))
	{
		DirectDiffuse[dispatchThreadId.xy] = 0.0f.xxxx;
		DirectSpecular[dispatchThreadId.xy] = 0.0f.xxxx;
		DirectSubsurface[dispatchThreadId.xy] = 0.0f.xxxx;
		return;
	}

	const float3 positionWorld = ReconstructGBufferWorldPosition(dispatchThreadId.xy, gBuffer.SceneDepth, InvViewMTX, InvProjectionMTX);
	const float3 viewDirWorld = normalize(Position - positionWorld);

	float3 directDiffuse = 0.0f;
	float3 directSpecular = 0.0f;
	float3 directSubsurface = 0.0f;
	const DirectLightReservoir::Reservoir reservoir =
	    DirectLightReservoir::UnpackReservoir(CurrentReservoirSample.Load(int3(dispatchThreadId.xy, 0)),
	                                          CurrentReservoirWeight.Load(int3(dispatchThreadId.xy, 0)));
	const ShadowVisibilitySignal shadowSignal =
	    RayTracedShadowSignalPacking::UnpackShadowSignal(ShadowVisibilitySignal.Load(int3(dispatchThreadId.xy, 0)));
	const bool evaluateSubsurface = any(gBuffer.SubsurfaceColor > 0.0f.xxx) && gBuffer.SubsurfaceStrength > 0.0f;
	if (DirectLightReservoir::IsValid(reservoir))
	{
		const LightSampling::DirectLightSample lightSample = DirectLightReservoir::ReplayLightSample(reservoir, positionWorld);
		const float reservoirWeight = DirectLightReservoir::GetFinalWeight(reservoir);

		AddDirectLightSample(gBuffer,
		                     viewDirWorld,
		                     evaluateSubsurface,
		                     lightSample,
		                     shadowSignal,
		                     reservoirWeight,
		                     directDiffuse,
		                     directSpecular,
		                     directSubsurface);
	}

	DirectDiffuse[dispatchThreadId.xy] = float4(directDiffuse, gBuffer.Alpha);
	DirectSpecular[dispatchThreadId.xy] = float4(directSpecular, gBuffer.Alpha);
	DirectSubsurface[dispatchThreadId.xy] = float4(directSubsurface, gBuffer.Alpha);
}
