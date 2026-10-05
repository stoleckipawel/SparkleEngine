#ifndef SPARKLE_RAY_TRACING_PATH_SAMPLING_HLSLI
#define SPARKLE_RAY_TRACING_PATH_SAMPLING_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/Common/Random.hlsli"
#include "/Engine/Lighting/SurfaceLighting.hlsli"
#include "/Engine/RayTracing/PathBsdf.hlsli"
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
		const uint4 words = CommonRandom::Philox4x32(uint4(pixelCoord, randomFrameIndex, sampleIndex), uint2(bounceIndex, 0x4C495450u));

		RandomSamples result;
		result.Lobe = CommonRandom::OpenUnitInterval(words.x);
		result.Direction = float2(CommonRandom::OpenUnitInterval(words.y), CommonRandom::OpenUnitInterval(words.z));
		result.Roulette = CommonRandom::OpenUnitInterval(words.w);
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

	RayTracingPathSample::DirectionSample InvalidSample()
	{
		RayTracingPathSample::DirectionSample result = (RayTracingPathSample::DirectionSample)0;
		return result;
	}

	RayTracingPathSample::DirectionSample SampleBSDF(RayTracingPathSurface surface,
	                                                 uint specularSampleMode,
	                                                 RandomSamples randomSamples,
	                                                 bool evaluateDiffuse,
	                                                 bool evaluateSpecular)
	{
		if (!surface.Valid)
		{
			return InvalidSample();
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
			return InvalidSample();
		}
		uint selectedLobe = RayTracingPathSample::LobeSpecular;
		if (randomSamples.Lobe < diffuseMass)
		{
			selectedLobe = RayTracingPathSample::LobeDiffuse;
		}

		return PathBsdf::Sample(surface, masses, selectedLobe, randomSamples.Direction, evaluateDiffuse, evaluateSpecular);
	}

	float RussianRouletteSurvivalProbability(float3 throughput, uint bounceIndex)
	{
		if (bounceIndex < 2u)
		{
			return 1.0f;
		}

		return saturate(max(max(throughput.r, throughput.g), throughput.b));
	}
}

#endif
