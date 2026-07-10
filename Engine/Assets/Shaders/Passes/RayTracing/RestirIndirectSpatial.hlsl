#include "Resources/ConstantBuffers.hlsli"
#include "Lighting/RestirIndirectLightingUniform.hlsli"

Texture2D<float4> TemporalReservoirSampleTexture;
Texture2D<float4> TemporalReservoirWeightTexture;
RWTexture2D<float4> CurrentReservoirSampleTexture;
RWTexture2D<float4> CurrentReservoirWeightTexture;
RWTexture2D<float4> CurrentReservoirSurfaceTexture;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

#include "Lighting/RestirIndirectReservoir.hlsli"

static const int2 SpatialOffsets[8] =
    {int2(1, 0), int2(-1, 0), int2(0, 1), int2(0, -1), int2(2, 1), int2(-2, 1), int2(2, -1), int2(-2, -1)};

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	CurrentReservoirSampleTexture.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const RestirIndirectReservoir::Surface surface = RestirIndirectReservoir::LoadSurface(pixelCoord);
	RestirIndirectReservoir::Reservoir reservoir = RestirIndirectReservoir::UnpackReservoir(
	    TemporalReservoirSampleTexture.Load(int3(pixelCoord, 0)),
	    TemporalReservoirWeightTexture.Load(int3(pixelCoord, 0)));

	if (surface.Valid)
	{
		uint rng = RestirReservoirCommon::BuildSeed(pixelCoord, 0x5A71A1u);
		const uint offsetStart = uint(CommonRandom::Random01(rng) * 8.0f) & 7u;
		[unroll] for (uint sampleIndex = 0u; sampleIndex < RestirReservoirCommon::SpatialNeighborCount; ++sampleIndex)
		{
			const int2 neighborCoord = int2(pixelCoord) + SpatialOffsets[(offsetStart + sampleIndex * 2u) & 7u];
			if (neighborCoord.x < 0 || neighborCoord.y < 0 || neighborCoord.x >= int(width) || neighborCoord.y >= int(height))
			{
				continue;
			}

			const RestirIndirectReservoir::Surface neighborSurface = RestirIndirectReservoir::LoadSurface(uint2(neighborCoord));
			if (!RestirIndirectReservoir::AreSurfacesCompatible(surface, RestirIndirectReservoir::PackSurface(neighborSurface)))
			{
				continue;
			}
			
			const RestirIndirectReservoir::Reservoir neighbor = RestirIndirectReservoir::UnpackReservoir(
			    TemporalReservoirSampleTexture.Load(int3(neighborCoord, 0)),
			    TemporalReservoirWeightTexture.Load(int3(neighborCoord, 0)));
			RestirIndirectReservoir::CombineReservoir(
			    reservoir,
			    neighbor,
			    surface,
			    SkyTexture,
			    SamplerLinearClamp,
			    RestirReservoirCommon::MaxSpatialM,
			    CommonRandom::Random01(rng));
		}
	}
	CurrentReservoirSampleTexture[pixelCoord] = RestirIndirectReservoir::PackSample(reservoir);
	CurrentReservoirWeightTexture[pixelCoord] = RestirIndirectReservoir::PackWeight(reservoir);
	CurrentReservoirSurfaceTexture[pixelCoord] = RestirIndirectReservoir::PackSurface(surface);
}
