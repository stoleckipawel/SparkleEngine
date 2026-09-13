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
		RayTracingEvaluatedTriangle Evaluated;
		MeshInstanceData Mesh;
		float3 P0;
		float3 P1;
		float3 P2;
		float3 Normal;
		float Area;
	};

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
		const float3 samplePosition = light.Position + tangent * ((sample.x - 0.5f) * light.Width)
		                            + bitangent * ((sample.y - 0.5f) * light.Height);
		LightSampling::DirectLightSample result = LightSampling::RadiometricAreaLightSample(
		    positionWorld, samplePosition, normal, RectRadiance(lightIndex), rcp(light.Width * light.Height), false);
		result.SamplePositionError = RayEndpoints::AnalyticPositionError(samplePosition, normal);
		return result;
	}

	EmissiveTriangle LoadEmissiveTriangle(uint instanceId, uint primitiveIndex)
	{
		const RayTracingEvaluatedTriangle evaluated = EvaluateRayTracingTriangle(instanceId, primitiveIndex, 0.0f.xx);
		EmissiveTriangle triangle;
		triangle.Instance = evaluated.Instance;
		triangle.Material = evaluated.Material;
		triangle.Evaluated = evaluated;
		triangle.Mesh = MeshInstances[instanceId];
		triangle.P0 = mul(float4(evaluated.V0.Position, 1.0f), triangle.Mesh.WorldMatrix).xyz;
		triangle.P1 = mul(float4(evaluated.V1.Position, 1.0f), triangle.Mesh.WorldMatrix).xyz;
		triangle.P2 = mul(float4(evaluated.V2.Position, 1.0f), triangle.Mesh.WorldMatrix).xyz;
		const float3 normalUnnormalized = cross(triangle.P1 - triangle.P0, triangle.P2 - triangle.P0);
		const float twiceArea = length(normalUnnormalized);
		triangle.Normal = normalUnnormalized / twiceArea;
		triangle.Area = 0.5f * twiceArea;
		return triangle;
	}

	LightSampling::DirectLightSample SampleEmissiveTriangle(float3 positionWorld,
	                                                        uint instanceId,
	                                                        uint primitiveIndex,
	                                                        float2 sample)
	{
		const EmissiveTriangle triangle = LoadEmissiveTriangle(instanceId, primitiveIndex);
		const float root = sqrt(sample.x);
		const float3 barycentrics = float3(1.0f - root, root * (1.0f - sample.y), root * sample.y);
		const float3 samplePosition =
		    triangle.P0 * barycentrics.x + triangle.P1 * barycentrics.y + triangle.P2 * barycentrics.z;
		const float2 barycentrics12 = barycentrics.yz;
		const RayTracingEvaluatedTriangle sampledTriangle =
		    EvaluateRayTracingTriangle(instanceId, primitiveIndex, barycentrics12);
		float3 emittedRadiance = triangle.Material.EmissiveColor;
		if (MaterialTextureTableSampling::HasTexture(
		        triangle.Material.TextureFlags, MaterialTextureTableSampling::TextureSlotEmissive))
		{
			emittedRadiance *= SampleRayTracingMaterialTexture(
			                       triangle.Material,
			                       MaterialTextureTableSampling::TextureSlotEmissive,
			                       InterpolateRayTracingTexCoord0(sampledTriangle))
			                       .rgb;
		}
		const bool twoSided = (triangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) != 0u;
		LightSampling::DirectLightSample result = LightSampling::RadiometricAreaLightSample(
		    positionWorld, samplePosition, triangle.Normal, emittedRadiance, rcp(triangle.Area), twoSided);
		const float3 positionObject = InterpolateRayTracingPosition(sampledTriangle);
		const float3 normalObject = cross(
		    sampledTriangle.V1.Position - sampledTriangle.V0.Position,
		    sampledTriangle.V2.Position - sampledTriangle.V0.Position);
		result.SamplePositionError = RayEndpoints::SurfaceErrorBound(
		    sampledTriangle, triangle.Mesh, positionObject, samplePosition, normalObject, triangle.Normal);
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
