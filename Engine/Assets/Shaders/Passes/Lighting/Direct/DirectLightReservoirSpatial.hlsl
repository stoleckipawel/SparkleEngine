#include "/Engine/Lighting/DirectLightReservoir.hlsli"

Texture2D<float4> TemporalReservoirSample;
Texture2D<float4> TemporalReservoirWeight;
RWTexture2D<float4> CurrentReservoirSample;
RWTexture2D<float4> CurrentReservoirWeight;
RWTexture2D<float4> CurrentReservoirSurface;

static const int2 SpatialOffsets[8] =
    {int2(1, 0), int2(-1, 0), int2(0, 1), int2(0, -1), int2(2, 1), int2(-2, 1), int2(2, -1), int2(-2, -1)};

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	CurrentReservoirSample.GetDimensions(width, height);

	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const DirectLightReservoir::Surface surface = DirectLightReservoir::LoadSurface(pixelCoord);
	if (!surface.Valid)
	{
		const DirectLightReservoir::Reservoir emptyReservoir = DirectLightReservoir::EmptyReservoir();
		CurrentReservoirSample[pixelCoord] = DirectLightReservoir::PackReservoirSample(emptyReservoir);
		CurrentReservoirWeight[pixelCoord] = DirectLightReservoir::PackReservoirWeight(emptyReservoir);
		CurrentReservoirSurface[pixelCoord] = 0.0f.xxxx;
		return;
	}

	DirectLightReservoir::Reservoir reservoir = DirectLightReservoir::UnpackReservoir(TemporalReservoirSample.Load(int3(pixelCoord, 0)),
	                                                                                  TemporalReservoirWeight.Load(int3(pixelCoord, 0)));

	uint rng = RestirReservoirCommon::BuildSeed(pixelCoord, 0x5A71A1u);
	const uint offsetStart = (uint)(CommonRandom::Random01(rng) * 8.0f) & 7u;
	[unroll] for (uint sampleIndex = 0u; sampleIndex < RestirReservoirCommon::SpatialNeighborCount; ++sampleIndex)
	{
		const int2 neighborCoord = int2(pixelCoord) + SpatialOffsets[(offsetStart + sampleIndex * 2u) & 7u];
		if (neighborCoord.x < 0 || neighborCoord.y < 0 || neighborCoord.x >= (int)width || neighborCoord.y >= (int)height)
		{
			continue;
		}

		const DirectLightReservoir::Surface neighborSurface = DirectLightReservoir::LoadSurface((uint2)neighborCoord);
		if (!DirectLightReservoir::AreSurfacesCompatible(surface, DirectLightReservoir::PackSurface(neighborSurface)))
		{
			continue;
		}

		const DirectLightReservoir::Reservoir neighborReservoir =
		    DirectLightReservoir::UnpackReservoir(TemporalReservoirSample.Load(int3(neighborCoord, 0)),
		                                          TemporalReservoirWeight.Load(int3(neighborCoord, 0)));
		DirectLightReservoir::CombineReservoir(reservoir,
		                                       neighborReservoir,
		                                       surface,
		                                       RestirReservoirCommon::MaxSpatialM,
		                                       CommonRandom::Random01(rng));
	}

	CurrentReservoirSample[pixelCoord] = DirectLightReservoir::PackReservoirSample(reservoir);
	CurrentReservoirWeight[pixelCoord] = DirectLightReservoir::PackReservoirWeight(reservoir);
	CurrentReservoirSurface[pixelCoord] = DirectLightReservoir::PackSurface(surface);
}
