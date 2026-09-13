#ifndef SPARKLE_RAY_TRACING_PATH_SAMPLING_HLSLI
#define SPARKLE_RAY_TRACING_PATH_SAMPLING_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/Common/Random.hlsli"
#include "/Engine/Lighting/SurfaceLighting.hlsli"
#include "/Engine/RayTracing/PathBsdf.hlsli"
#include "/Engine/RayTracing/PathTracer.hlsli"
#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayTracingPathSample.hlsli"

namespace RayTracingPathSampling
{
	static const uint SpecularSampleModeStochasticGGX = 1u;

	struct RandomSamples
	{
		float Lobe;
		float2 Direction;
		float Roulette;
	};

	RandomSamples GenerateRandomSamples(uint2 pixelCoord, uint bounceIndex, uint sampleIndex, uint randomFrameIndex)
	{
		const uint sampleSalt = sampleIndex * 4099u;
		const float2 basePixel = float2(pixelCoord) + float2(bounceIndex * 17u + sampleSalt, bounceIndex * 29u + sampleSalt);
		const float2 lobeAndRoulette =
		    CommonRandom::InterleavedGradientNoise2(basePixel, randomFrameIndex + bounceIndex * 131u + sampleSalt, float2(211.0f, 97.0f));
		const float2 direction =
		    CommonRandom::InterleavedGradientNoise2(basePixel, randomFrameIndex + bounceIndex * 149u + sampleSalt, float2(41.0f, 137.0f));

		RandomSamples result;
		result.Lobe = lobeAndRoulette.x;
		result.Roulette = lobeAndRoulette.y;
		result.Direction = direction;
		return result;
	}

	RandomSamples GenerateRandomSamples(uint2 pixelCoord, uint bounceIndex, uint sampleIndex)
	{
		return GenerateRandomSamples(pixelCoord, bounceIndex, sampleIndex, FrameIndex);
	}

	float DiffuseLobeProbability(RayTracingPathSurface surface, float3 f0)
	{
		const float3 luminanceWeights = float3(0.2126f, 0.7152f, 0.0722f);
		const float diffuseWeight = max(dot(surface.BaseColor, luminanceWeights) * (1.0f - surface.Metallic), 0.0f);
		const float specularWeight = max(dot(f0, luminanceWeights), 0.0f);
		const float totalWeight = diffuseWeight + specularWeight;
		return totalWeight > 0.0f ? saturate(diffuseWeight / totalWeight) : 0.5f;
	}

	RayTracingPathSample::DirectionSample InvalidSample(uint lobe)
	{
		RayTracingPathSample::DirectionSample result = (RayTracingPathSample::DirectionSample)0;
		result.Lobe = lobe;
		result.RejectionReason = RayTracingPathSample::RejectionReasonInvalidSample;
		return result;
	}

	RayTracingPathSample::DirectionSample SampleBSDF(RayTracingPathSurface surface, uint specularSampleMode, RandomSamples randomSamples)
	{
		if (!surface.Valid)
		{
			return InvalidSample(RayTracingPathSample::LobeNone);
		}

		const float3 f0 = SurfaceLighting::BuildF0(surface.BaseColor, surface.Metallic, surface.DielectricF0);
		const float diffuseMass = DiffuseLobeProbability(surface, f0);
		PathBsdf::LobeMasses masses;
		masses.Diffuse = diffuseMass;
		masses.Specular = 1.0f - diffuseMass;
		masses.Count = (masses.Diffuse > 0.0f ? 1u : 0u) + (masses.Specular > 0.0f ? 1u : 0u);
		masses.SpecularDelta = surface.Roughness == 0.0f;
		if (specularSampleMode != SpecularSampleModeStochasticGGX && !masses.SpecularDelta)
		{
			return InvalidSample(RayTracingPathSample::LobeSpecular);
		}
		const uint selectedLobe = randomSamples.Lobe < diffuseMass ? RayTracingPathSample::LobeDiffuse
		                                                            : RayTracingPathSample::LobeSpecular;
		return PathBsdf::Sample(surface, masses, selectedLobe, randomSamples.Direction);
	}

	bool SurvivesRussianRoulette(inout float3 throughput, float randomValue, uint bounceIndex)
	{
		if (bounceIndex < 2u)
		{
			return true;
		}

		const float survivalProbability = saturate(max(max(throughput.r, throughput.g), throughput.b));
		if (survivalProbability <= 0.0f || randomValue > survivalProbability)
		{
			return false;
		}

		PathTracer::ApplySurvivalCompensation(throughput, survivalProbability);
		return true;
	}
}

#endif
