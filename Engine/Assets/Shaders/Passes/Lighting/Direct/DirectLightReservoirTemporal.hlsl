#include "/Engine/Resources/ViewTemporalUniformData.hlsli"

#include "/Engine/Lighting/DirectLightReservoir.hlsli"
#include "/Engine/Passes/GBuffer/MotionVector.hlsli"

RWTexture2D<float4> TemporalReservoirSample;
RWTexture2D<float4> TemporalReservoirWeight;
Texture2D<float4> PreviousReservoirSample;
Texture2D<float4> PreviousReservoirWeight;
Texture2D<float4> PreviousReservoirSurface;
Texture2D GBufferMotionVector;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	TemporalReservoirSample.GetDimensions(width, height);

	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const DirectLightReservoir::Surface surface = DirectLightReservoir::LoadSurface(pixelCoord);
	if (!surface.Valid)
	{
		TemporalReservoirSample[pixelCoord] = DirectLightReservoir::PackReservoirSample(DirectLightReservoir::EmptyReservoir());
		TemporalReservoirWeight[pixelCoord] = DirectLightReservoir::PackReservoirWeight(DirectLightReservoir::EmptyReservoir());
		return;
	}

	DirectLightReservoir::Reservoir reservoir = DirectLightReservoir::BuildInitialReservoir(surface, pixelCoord);

	if (HistoryValid != 0u)
	{
		const float2 motionPixels = GBufferMotionVector.Load(int3(pixelCoord, 0)).xy;
		const float2 previousPixel = MotionVectors::ReprojectToPreviousPixelCenter(pixelCoord, motionPixels, float2(width, height));
		const int2 previousPixelCoord = int2(floor(previousPixel));
		if (previousPixelCoord.x >= 0 && previousPixelCoord.y >= 0 && previousPixelCoord.x < (int)width
		    && previousPixelCoord.y < (int)height)
		{
			const float4 previousSurface = PreviousReservoirSurface.Load(int3(previousPixelCoord, 0));
			if (DirectLightReservoir::AreSurfacesCompatible(surface, previousSurface))
			{
				const DirectLightReservoir::Reservoir previousReservoir =
				    DirectLightReservoir::UnpackReservoir(PreviousReservoirSample.Load(int3(previousPixelCoord, 0)),
				                                          PreviousReservoirWeight.Load(int3(previousPixelCoord, 0)));
				uint rng = RestirReservoirCommon::BuildSeed(pixelCoord, 0x7151u);
				DirectLightReservoir::CombineReservoir(reservoir,
				                                       previousReservoir,
				                                       surface,
				                                       RestirReservoirCommon::MaxTemporalM,
				                                       CommonRandom::Random01(rng));
			}
		}
	}

	TemporalReservoirSample[pixelCoord] = DirectLightReservoir::PackReservoirSample(reservoir);
	TemporalReservoirWeight[pixelCoord] = DirectLightReservoir::PackReservoirWeight(reservoir);
}
