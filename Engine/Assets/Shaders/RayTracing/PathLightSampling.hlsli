#ifndef SPARKLE_RAY_TRACING_PATH_LIGHT_SAMPLING_HLSLI
#define SPARKLE_RAY_TRACING_PATH_LIGHT_SAMPLING_HLSLI

#include "/Engine/Resources/LightGpuData.hlsli"
#include "/Engine/Resources/MeshInstanceShaderData.hlsli"
#include "/Engine/Resources/SceneLightingUniformData.hlsli"

#include "/Engine/Lighting/LightSampling.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialHit.hlsli"

namespace PathLightSampling
{
	struct EmissiveTriangle
	{
		RayTracingHitInstance Instance;
		RayTracingHitMaterial Material;
		MeshInstanceData Mesh;
		float3 P0;
		float3 P1;
		float3 P2;
		float3 Normal;
		float Area;
	};

	uint CountAnalyticLights()
	{
		return SceneLighting.DirectionalLightCount + SceneLighting.PointLightCount + SceneLighting.SpotLightCount
		    + SceneLighting.RectLightCount;
	}

	uint CountEmissiveTriangles(uint instanceId)
	{
		const RayTracingHitInstance instance = RayTracingHitInstances[instanceId];
		return (instance.Flags & RayTracingHitSurface::InstanceFlagValid) != 0u
		        && (RayTracingHitMaterials[instance.MaterialSlot].Flags & RayTracingHitSurface::MaterialFlagEmissive) != 0u
		    ? instance.IndexCount / 3u
		    : 0u;
	}

	uint CountEmissiveTriangles()
	{
		uint count = 0u;
		[loop] for (uint instanceId = 0u; instanceId < RayTracingHitInstanceCount; ++instanceId)
		{
			count += CountEmissiveTriangles(instanceId);
		}
		return count;
	}

	void FindAnalyticLight(uint ordinal, out uint lightType, out uint lightIndex)
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
			const uint primitiveCount = CountEmissiveTriangles(candidateInstance);
			if (ordinal < primitiveCount)
			{
				instanceId = candidateInstance;
				primitiveIndex = ordinal;
				return;
			}
			ordinal -= primitiveCount;
		}
	}

	float3 DirectionalIrradiance(uint lightIndex)
	{
		const DirectionalLightGpuData light = DirectionalLights[lightIndex];
		return LightSampling::PhotometricRgbToRadiometric(light.Color, light.Illuminance);
	}

	float3 PointIntensity(uint lightIndex)
	{
		const PointLightGpuData light = PointLights[lightIndex];
		return LightSampling::PhotometricRgbToRadiometric(light.Color, light.LuminousIntensity);
	}

	float3 SpotIntensity(uint lightIndex)
	{
		const SpotLightGpuData light = SpotLights[lightIndex];
		return LightSampling::PhotometricRgbToRadiometric(light.Color, light.LuminousIntensity);
	}

	float3 RectRadiance(uint lightIndex)
	{
		const RectLightGpuData light = RectLights[lightIndex];
		return LightSampling::PhotometricRgbToRadiometric(light.Color, light.Luminance);
	}

	LightSampling::DirectLightSample SampleAnalyticLight(uint lightType, uint lightIndex, float3 positionWorld, float2 sample)
	{
		if (lightType == LightSampling::LightTypeDirectional)
		{
			return LightSampling::RadiometricDirectionalLightSample(-DirectionalLights[lightIndex].Direction,
			                                                        DirectionalIrradiance(lightIndex));
		}
		if (lightType == LightSampling::LightTypePoint)
		{
			return LightSampling::RadiometricPointLightSample(positionWorld, PointLights[lightIndex].Position, PointIntensity(lightIndex));
		}
		if (lightType == LightSampling::LightTypeSpot)
		{
			const SpotLightGpuData light = SpotLights[lightIndex];
			LightSampling::DirectLightSample result =
			    LightSampling::RadiometricPointLightSample(positionWorld, light.Position, SpotIntensity(lightIndex));
			const float coneCosine = dot(normalize(light.Direction), -result.DirectionWorld);
			const float angular = light.InnerAngleCosine == light.OuterAngleCosine
			    ? (coneCosine >= light.OuterAngleCosine ? 1.0f : 0.0f)
			    : smoothstep(light.OuterAngleCosine, light.InnerAngleCosine, coneCosine);
			result.IncidentRadiance *= angular;
			return result;
		}

		const RectLightGpuData light = RectLights[lightIndex];
		const float3 normal = normalize(light.Direction);
		const float3 tangent = normalize(light.Tangent - normal * dot(light.Tangent, normal));
		const float3 bitangent = cross(normal, tangent);
		const float3 samplePosition =
		    light.Position + tangent * ((sample.x - 0.5f) * light.Width) + bitangent * ((sample.y - 0.5f) * light.Height);
		LightSampling::DirectLightSample result = LightSampling::RadiometricAreaLightSample(positionWorld,
		                                                                                    samplePosition,
		                                                                                    normal,
		                                                                                    RectRadiance(lightIndex),
		                                                                                    rcp(light.Width * light.Height),
		                                                                                    false);
		return result;
	}

	EmissiveTriangle LoadEmissiveTriangle(uint instanceId, uint primitiveIndex)
	{
		const RayTracingEvaluatedTriangle evaluated = EvaluateRayTracingTriangle(instanceId, primitiveIndex, 0.0f.xx);
		EmissiveTriangle triangle;
		triangle.Instance = evaluated.Instance;
		triangle.Material = evaluated.Material;
		triangle.Mesh = MeshInstances[instanceId];
		triangle.P0 = RayEndpoints::TransformPosition(evaluated.V0.Position, triangle.Mesh.WorldMatrix);
		triangle.P1 = RayEndpoints::TransformPosition(evaluated.V1.Position, triangle.Mesh.WorldMatrix);
		triangle.P2 = RayEndpoints::TransformPosition(evaluated.V2.Position, triangle.Mesh.WorldMatrix);
		const float3 normalUnnormalized = cross(triangle.P1 - triangle.P0, triangle.P2 - triangle.P0);
		const float3 normalObject = cross(evaluated.V1.Position - evaluated.V0.Position, evaluated.V2.Position - evaluated.V0.Position);
		const float twiceArea = length(normalUnnormalized);
		triangle.Normal = RayEndpoints::TransformGeometricNormal(normalObject, triangle.Mesh);
		triangle.Area = 0.5f * twiceArea;
		return triangle;
	}

	LightSampling::DirectLightSample SampleEmissiveTriangle(float3 positionWorld, uint instanceId, uint primitiveIndex, float2 sample)
	{
		const EmissiveTriangle triangle = LoadEmissiveTriangle(instanceId, primitiveIndex);
		const float root = sqrt(sample.x);
		const float3 barycentrics = float3(1.0f - root, root * (1.0f - sample.y), root * sample.y);
		const float2 barycentrics12 = barycentrics.yz;
		const RayTracingEvaluatedTriangle sampledTriangle = EvaluateRayTracingTriangle(instanceId, primitiveIndex, barycentrics12);
		const float3 positionObject = InterpolateRayTracingPosition(sampledTriangle);
		const float3 samplePosition = RayEndpoints::TransformPosition(positionObject, triangle.Mesh.WorldMatrix);
		if (!PassesRayTracingMaterialAlpha(triangle.Material,
		                                   InterpolateRayTracingTexCoord0(sampledTriangle),
		                                   InterpolateRayTracingColor(sampledTriangle)))
		{
			return (LightSampling::DirectLightSample)0;
		}
		float3 emittedRadiance = triangle.Material.EmissiveColor;
		if (MaterialTextureTableSampling::HasTexture(triangle.Material.TextureFlags, MaterialTextureTableSampling::TextureSlotEmissive))
		{
			emittedRadiance *= SampleRayTracingMaterialTexture(triangle.Material,
			                                                   MaterialTextureTableSampling::TextureSlotEmissive,
			                                                   InterpolateRayTracingTexCoord0(sampledTriangle))
			                       .rgb;
		}
		const bool twoSided = (triangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) != 0u;
		LightSampling::DirectLightSample result = LightSampling::RadiometricAreaLightSample(positionWorld,
		                                                                                    samplePosition,
		                                                                                    triangle.Normal,
		                                                                                    emittedRadiance,
		                                                                                    rcp(triangle.Area),
		                                                                                    twoSided);
		const float3 normalObject =
		    cross(sampledTriangle.V1.Position - sampledTriangle.V0.Position, sampledTriangle.V2.Position - sampledTriangle.V0.Position);
		const RayEndpoints::SurfaceEndpointError endpointError =
		    RayEndpoints::BuildSurfaceEndpointError(sampledTriangle, triangle.Mesh, positionObject, normalObject);
		result.EmitterEndpointBaseOffset = endpointError.BaseOffset;
		result.EmitterEndpointTraversalSensitivity = endpointError.TraversalSensitivity;
		result.TargetInstanceId = instanceId;
		result.TargetPrimitiveIndex = primitiveIndex;
		return result;
	}

	float EmissiveTrianglePdfW(float3 positionWorld, float3 hitPositionWorld, uint instanceId, uint primitiveIndex)
	{
		const EmissiveTriangle triangle = LoadEmissiveTriangle(instanceId, primitiveIndex);
		const bool twoSided = (triangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) != 0u;
		const LightSampling::DirectLightSample sample = LightSampling::RadiometricAreaLightSample(positionWorld,
		                                                                                          hitPositionWorld,
		                                                                                          triangle.Normal,
		                                                                                          triangle.Material.EmissiveColor,
		                                                                                          rcp(triangle.Area),
		                                                                                          twoSided);
		return sample.PdfW;
	}
}

#endif
