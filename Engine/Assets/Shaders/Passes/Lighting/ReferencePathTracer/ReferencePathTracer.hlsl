#include "/Engine/Resources/ViewCameraRay.hlsli"

#include "/Engine/RayTracing/PathAccumulation.hlsli"
#include "/Engine/RayTracing/RayEndpoints.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerSampler.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerTransport.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerUniformData.hlsli"

RWTexture2D<float4> WorkingMean;
RWTexture2D<float4> WorkingM2;
Texture2D<float4> CommittedMean;
Texture2D<float4> CommittedM2;
RaytracingAccelerationStructure SceneTlas;
Texture2D SkyTexture;
SamplerState SamplerLinearWrapClamp;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width;
	uint height;
	WorkingMean.GetDimensions(width, height);
	const uint2 pixelCoord = uint2(dispatchThreadId.x, FirstRow + dispatchThreadId.y);
	if (pixelCoord.x >= width || pixelCoord.y >= height || dispatchThreadId.y >= RowCount
	    || (WorkFlags & ReferencePathTracerWorkFlag_Trace) == 0u)
	{
		return;
	}

	ReferencePathTracer::SampleIdentity sampleIdentity;
	sampleIdentity.PixelCoord = pixelCoord;
	sampleIdentity.SampleOrdinal = SampleOrdinal;
	sampleIdentity.SessionSeed = SessionSeed;
	sampleIdentity.ReplicateId = ReplicateId;
	const float2 filmSample =
	    float2(CommonRandom::OpenUnitInterval(ReferencePathTracer::RandomWord(sampleIdentity, ReferencePathTracer::FilmXDimension)),
	           CommonRandom::OpenUnitInterval(ReferencePathTracer::RandomWord(sampleIdentity, ReferencePathTracer::FilmYDimension)));
	const ViewCameraRay cameraRay = BuildPerspectiveViewCameraRay(pixelCoord, uint2(width, height), filmSample);
	const RayEndpoints::Ray primaryRay =
	    RayEndpoints::Primary(cameraRay.OriginWorld, cameraRay.DirectionWorld, cameraRay.TMin, cameraRay.TMax);
	const float3 contribution =
	    ReferencePathTracer::TraceSurfaceTransport(SceneTlas, SkyTexture, SamplerLinearWrapClamp, primaryRay, sampleIdentity);

	float3 mean = PriorSampleCount == 0u ? 0.0f.xxx : CommittedMean[pixelCoord].rgb;
	float3 m2 = PriorSampleCount == 0u ? 0.0f.xxx : CommittedM2[pixelCoord].rgb;
	PathAccumulation::AddSample(mean, m2, PriorSampleCount, contribution);
	WorkingMean[pixelCoord] = float4(mean, 1.0f);
	WorkingM2[pixelCoord] = float4(m2, 0.0f);
}
