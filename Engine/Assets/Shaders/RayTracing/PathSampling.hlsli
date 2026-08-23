#ifndef SPARKLE_RAY_TRACING_PATH_SAMPLING_HLSLI
#define SPARKLE_RAY_TRACING_PATH_SAMPLING_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/BRDF/BRDF.hlsli"
#include "/Engine/BRDF/SpecularSampling.hlsli"
#include "/Engine/Common/Random.hlsli"
#include "/Engine/Common/Sampling.hlsli"
#include "/Engine/Lighting/SurfaceLighting.hlsli"
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
		RayTracingPathSample::DirectionSample result;
		result.DirectionWorld = 0.0f.xxx;
		result.Pdf = 0.0f;
		result.CosineTerm = 0.0f;
		result.Throughput = 0.0f.xxx;
		result.Lobe = lobe;
		result.Mirror = false;
		result.RejectionReason = RayTracingPathSample::RejectionReasonInvalidSample;
		return result;
	}

	RayTracingPathSample::DirectionSample SampleDiffuseLobe(RayTracingPathSurface surface, float lobePdf, float2 randomSample)
	{
		const CommonSampling::CosineHemisphereSample cosineSample =
		    CommonSampling::SampleCosineHemisphere(surface.NormalWorld, randomSample);
		RayTracingPathSample::DirectionSample result = InvalidSample(RayTracingPathSample::LobeDiffuse);
		result.DirectionWorld = cosineSample.DirectionWorld;
		result.Pdf = cosineSample.Pdf * lobePdf;
		result.CosineTerm = cosineSample.Cosine;

		const BRDF::ShadingData shadingData = BRDF::ComputeShadingData(surface.NormalWorld, surface.ViewDirWorld, result.DirectionWorld);
		if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f || result.Pdf <= 0.0f)
		{
			return result;
		}

		const float3 f0 = SurfaceLighting::BuildF0(surface.BaseColor, surface.Metallic, surface.DielectricF0);
		const float3 fresnel = BRDF::Fresnel::EvaluateDirect(shadingData.VoH, f0);
		const float3 diffuseWeight = (1.0f.xxx - fresnel) * (1.0f - surface.Metallic);
		const float3 diffuseBrdf = BRDF::Diffuse::EvaluateDirect(surface.BaseColor, surface.Roughness, shadingData) * diffuseWeight;

		result.Throughput = max(diffuseBrdf * (result.CosineTerm / result.Pdf), 0.0f.xxx);
		result.RejectionReason = RayTracingPathSample::RejectionReasonNone;
		return result;
	}

	RayTracingPathSample::DirectionSample SampleSpecularLobe(RayTracingPathSurface surface,
	                                                         uint sampleMode,
	                                                         float lobePdf,
	                                                         float2 randomSample)
	{
		const BRDF::SpecularSampling::LobeSample specularSample = BRDF::SpecularSampling::SampleReflectionLobe(surface.NormalWorld,
		                                                                                                       surface.ViewDirWorld,
		                                                                                                       surface.Roughness,
		                                                                                                       sampleMode,
		                                                                                                       randomSample);

		RayTracingPathSample::DirectionSample result = InvalidSample(RayTracingPathSample::LobeSpecular);
		result.DirectionWorld = specularSample.DirectionWorld;
		result.Pdf = specularSample.Pdf * lobePdf;
		result.Mirror = specularSample.Mirror;
		if (!specularSample.Valid || result.Pdf <= 0.0f)
		{
			return result;
		}

		const BRDF::ShadingData shadingData = BRDF::ComputeShadingData(surface.NormalWorld, surface.ViewDirWorld, result.DirectionWorld);
		result.CosineTerm = shadingData.NoL;
		if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f)
		{
			return result;
		}

		const float3 f0 = SurfaceLighting::BuildF0(surface.BaseColor, surface.Metallic, surface.DielectricF0);
		const float3 fresnel = BRDF::Fresnel::EvaluateDirect(shadingData.VoH, f0);
		result.Throughput = specularSample.Mirror
		    ? max(fresnel / lobePdf, 0.0f.xxx)
		    : max(BRDF::Specular::EvaluateDirect(shadingData, surface.Roughness, fresnel) * (shadingData.NoL / result.Pdf), 0.0f.xxx);
		result.RejectionReason = RayTracingPathSample::RejectionReasonNone;
		return result;
	}

	RayTracingPathSample::DirectionSample SampleBSDF(RayTracingPathSurface surface, uint specularSampleMode, RandomSamples randomSamples)
	{
		if (!surface.Valid)
		{
			return InvalidSample(RayTracingPathSample::LobeNone);
		}

		const float3 f0 = SurfaceLighting::BuildF0(surface.BaseColor, surface.Metallic, surface.DielectricF0);
		const float diffusePdf = DiffuseLobeProbability(surface, f0);
		if (randomSamples.Lobe < diffusePdf)
		{
			if (diffusePdf <= 0.0f)
			{
				return InvalidSample(RayTracingPathSample::LobeDiffuse);
			}
			return SampleDiffuseLobe(surface, diffusePdf, randomSamples.Direction);
		}

		const float specularPdf = 1.0f - diffusePdf;
		if (specularPdf <= 0.0f)
		{
			return InvalidSample(RayTracingPathSample::LobeSpecular);
		}
		return SampleSpecularLobe(surface, specularSampleMode, specularPdf, randomSamples.Direction);
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

		throughput /= survivalProbability;
		return true;
	}
}

#endif
