#ifndef SPARKLE_REFERENCE_PATH_TRACER_LIGHT_SAMPLING_HLSLI
#define SPARKLE_REFERENCE_PATH_TRACER_LIGHT_SAMPLING_HLSLI

#include "/Engine/Common/Random.hlsli"
#include "/Engine/Lighting/EnvironmentSampling.hlsli"
#include "/Engine/RayTracing/PathLightSampling.hlsli"

namespace ReferencePathTracerLightSampling
{
	static const uint KindAnalytic = 0u;
	static const uint KindEmissiveTriangle = 1u;
	static const uint KindEnvironment = 2u;

	struct Counts
	{
		uint Analytic;
		uint EmissiveTriangle;
		uint Environment;
		uint Total;
	};

	struct Selection
	{
		uint Kind;
		uint LightType;
		uint LightIndex;
		uint InstanceId;
		uint PrimitiveIndex;
		float Probability;
	};

	uint CountAnalyticLights()
	{
		return SceneLighting.DirectionalLightCount + SceneLighting.PointLightCount + SceneLighting.SpotLightCount
		     + SceneLighting.RectLightCount;
	}

	uint CountEmissiveTriangles()
	{
		uint count = 0u;
		[loop] for (uint instanceId = 0u; instanceId < RayTracingHitInstanceCount; ++instanceId)
		{
			const RayTracingHitInstance instance = RayTracingHitInstances[instanceId];
			if ((RayTracingHitMaterials[instance.MaterialSlot].Flags & RayTracingHitSurface::MaterialFlagEmissive) != 0u)
			{
				count += instance.IndexCount / 3u;
			}
		}
		return count;
	}

	Counts GetCounts()
	{
		Counts result;
		result.Analytic = CountAnalyticLights();
		result.EmissiveTriangle = CountEmissiveTriangles();
		result.Environment = SkyEnabled;
		result.Total = result.Analytic + result.EmissiveTriangle + result.Environment;
		return result;
	}

	void FindAnalytic(uint ordinal, out uint lightType, out uint lightIndex)
	{
		lightType = LightSampling::LightTypeDirectional;
		lightIndex = 0u;
		if (ordinal < SceneLighting.DirectionalLightCount)
		{
			lightIndex = ordinal;
			return;
		}
		ordinal -= SceneLighting.DirectionalLightCount;
		if (ordinal < SceneLighting.PointLightCount)
		{
			lightType = LightSampling::LightTypePoint;
			lightIndex = ordinal;
			return;
		}
		ordinal -= SceneLighting.PointLightCount;
		if (ordinal < SceneLighting.SpotLightCount)
		{
			lightType = LightSampling::LightTypeSpot;
			lightIndex = ordinal;
			return;
		}
		lightType = LightSampling::LightTypeRect;
		lightIndex = ordinal - SceneLighting.SpotLightCount;
	}

	void FindEmissiveTriangle(uint ordinal, out uint instanceId, out uint primitiveIndex)
	{
		instanceId = 0u;
		primitiveIndex = 0u;
		[loop] for (uint candidateInstance = 0u; candidateInstance < RayTracingHitInstanceCount; ++candidateInstance)
		{
			const RayTracingHitInstance instance = RayTracingHitInstances[candidateInstance];
			const uint primitiveCount =
			    (RayTracingHitMaterials[instance.MaterialSlot].Flags & RayTracingHitSurface::MaterialFlagEmissive) != 0u
			        ? instance.IndexCount / 3u
			        : 0u;
			if (ordinal < primitiveCount)
			{
				instanceId = candidateInstance;
				primitiveIndex = ordinal;
				return;
			}
			ordinal -= primitiveCount;
		}
	}

	Selection Select(Counts counts, uint word)
	{
		Selection result = (Selection)0;
		const CommonRandom::CategoricalSample category = CommonRandom::SampleCategorical(word, counts.Total);
		result.Probability = category.ProbabilityMass;
		uint ordinal = category.Index;
		if (ordinal < counts.Analytic)
		{
			result.Kind = KindAnalytic;
			FindAnalytic(ordinal, result.LightType, result.LightIndex);
			return result;
		}
		ordinal -= counts.Analytic;
		if (ordinal < counts.EmissiveTriangle)
		{
			result.Kind = KindEmissiveTriangle;
			FindEmissiveTriangle(ordinal, result.InstanceId, result.PrimitiveIndex);
			return result;
		}
		result.Kind = KindEnvironment;
		return result;
	}

	LightSampling::DirectLightSample Sample(Selection selection,
	                                        float3 positionWorld,
	                                        float2 shapeSample,
	                                        Texture2D skyTexture,
	                                        SamplerState skySampler)
	{
		LightSampling::DirectLightSample result;
		if (selection.Kind == KindAnalytic)
		{
			result = PathLightSampling::SampleAnalyticLight(
			    selection.LightType, selection.LightIndex, positionWorld, shapeSample);
		}
		else if (selection.Kind == KindEmissiveTriangle)
		{
			result = PathLightSampling::SampleEmissiveTriangle(
			    positionWorld, selection.InstanceId, selection.PrimitiveIndex, shapeSample);
		}
		else
		{
			result = SampleUniformEnvironmentRadiance(skyTexture, skySampler, shapeSample);
		}
		result.LightSelectionPdf = selection.Probability;
		return result;
	}

	float EnvironmentPdfW(Counts counts)
	{
		return counts.Environment == 0u
		    ? 0.0f
		    : CommonRandom::CategoricalProbabilityMass(counts.Analytic + counts.EmissiveTriangle, counts.Total)
		          * 0.07957747154594766788f;
	}

	float EmissiveTrianglePdfW(Counts counts, float3 previousPositionWorld, RayTracingPathSurface surface)
	{
		uint ordinal = surface.PrimitiveIndex;
		[loop] for (uint instanceId = 0u; instanceId < surface.InstanceId; ++instanceId)
		{
			const RayTracingHitInstance instance = RayTracingHitInstances[instanceId];
			if ((RayTracingHitMaterials[instance.MaterialSlot].Flags & RayTracingHitSurface::MaterialFlagEmissive) != 0u)
			{
				ordinal += instance.IndexCount / 3u;
			}
		}
		return CommonRandom::CategoricalProbabilityMass(counts.Analytic + ordinal, counts.Total)
		     * PathLightSampling::EmissiveTrianglePdfW(
		         previousPositionWorld, surface.PositionWorld, surface.InstanceId, surface.PrimitiveIndex);
	}
}

#endif
