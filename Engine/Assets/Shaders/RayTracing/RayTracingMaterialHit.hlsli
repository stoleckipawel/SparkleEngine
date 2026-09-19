#pragma once

#include "/Engine/Geometry/Basis.hlsli"
#include "/Engine/Material/MaterialNormal.hlsli"
#include "/Engine/RayTracing/RayEndpoints.hlsli"
#include "/Engine/RayTracing/RayTracingHitSurface.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialAlpha.hlsli"
#include "/Engine/RayTracing/RayTracingTraceResult.hlsli"

struct RayTracingMaterialSample
{
	float4 BaseColor;
	float3 Emissive;
	float3 NormalTangent;
	float3 SubsurfaceColor;
	float Roughness;
	float Metallic;
	float AmbientOcclusion;
	float SubsurfaceStrength;
};

RayTracingMaterialSample EvaluateRayTracingHitMaterial(RayTracingHitMaterial material, float2 uv, float4 vertexColor)
{
	RayTracingMaterialSample result;
	result.BaseColor = material.BaseColor;
	result.Emissive = material.EmissiveColor;
	result.NormalTangent = float3(0.0f, 0.0f, 1.0f);
	result.SubsurfaceColor = material.SubsurfaceColor;
	result.Roughness = material.Roughness;
	result.Metallic = material.Metallic;
	result.AmbientOcclusion = 1.0f;
	result.SubsurfaceStrength = material.SubsurfaceStrength;

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotNormal))
	{
		const MaterialTextureMappingData mapping = material.TextureMappings[MaterialTextureTableSampling::TextureSlotNormal];

		result.NormalTangent =
		    UnpackMaterialNormal(SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotNormal, uv).rgb,
		                         mapping.Strength);
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotBaseColor))
	{
		result.BaseColor =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotBaseColor, uv) * material.BaseColor;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotRoughness))
	{
		result.Roughness =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotRoughness, uv).r * material.Roughness;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotMetallic))
	{
		result.Metallic =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotMetallic, uv).r * material.Metallic;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotEmissive))
	{
		result.Emissive =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotEmissive, uv).rgb * material.EmissiveColor;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotOcclusion))
	{
		const MaterialTextureMappingData mapping = material.TextureMappings[MaterialTextureTableSampling::TextureSlotOcclusion];
		const float sampledOcclusion = SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotOcclusion, uv).r;

		result.AmbientOcclusion = lerp(1.0f, sampledOcclusion, mapping.Strength);
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotSubsurfaceColor))
	{
		result.SubsurfaceColor = SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotSubsurfaceColor, uv).rgb
		    * material.SubsurfaceColor;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotSubsurfaceStrength))
	{
		result.SubsurfaceStrength =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotSubsurfaceStrength, uv).r
		    * material.SubsurfaceStrength;
	}

	result.BaseColor *= vertexColor;
	return result;
}

RayTracingHitSurfaceData ReconstructRayTracingHitSurface(RayTracingTraceResult trace, float3 rayDirectionWorld)
{
	const RayTracingEvaluatedTriangle evaluatedTriangle =
	    EvaluateRayTracingTriangle(trace.InstanceId, trace.PrimitiveIndex, trace.Barycentrics);
	const MeshInstanceData mesh = MeshInstances[trace.InstanceId];
	const float3 positionObject = InterpolateRayTracingPosition(evaluatedTriangle);
	const float3 positionWorld = RayEndpoints::TransformPosition(positionObject, mesh.WorldMatrix);
	const float3 normalObject =
	    cross(evaluatedTriangle.V1.Position - evaluatedTriangle.V0.Position, evaluatedTriangle.V2.Position - evaluatedTriangle.V0.Position);
	const float3 outwardGeometricNormal = RayEndpoints::TransformGeometricNormal(normalObject, mesh);

	const bool twoSided = (evaluatedTriangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) != 0u;

	RayTracingHitSurfaceData surface = (RayTracingHitSurfaceData)0;
	if (!trace.FrontFace && !twoSided)
	{
		surface.RejectionReason = RayTracingHitSurface::ReasonOneSidedBackface;
		return surface;
	}

	const float faceSign = trace.FrontFace ? 1.0f : -1.0f;
	const float3 geometricNormal = outwardGeometricNormal * faceSign;

	const float3 localNormal = evaluatedTriangle.V0.Normal * evaluatedTriangle.BarycentricWeights.x
	    + evaluatedTriangle.V1.Normal * evaluatedTriangle.BarycentricWeights.y
	    + evaluatedTriangle.V2.Normal * evaluatedTriangle.BarycentricWeights.z;
	const float3 localTangent = evaluatedTriangle.V0.Tangent * evaluatedTriangle.BarycentricWeights.x
	    + evaluatedTriangle.V1.Tangent * evaluatedTriangle.BarycentricWeights.y
	    + evaluatedTriangle.V2.Tangent * evaluatedTriangle.BarycentricWeights.z;
	const float interpolatedTangentSign = evaluatedTriangle.V0.TangentSign * evaluatedTriangle.BarycentricWeights.x
	    + evaluatedTriangle.V1.TangentSign * evaluatedTriangle.BarycentricWeights.y
	    + evaluatedTriangle.V2.TangentSign * evaluatedTriangle.BarycentricWeights.z;
	const float tangentSign = interpolatedTangentSign >= 0.0f ? 1.0f : -1.0f;

	float3 vertexNormal = normalize(mul(localNormal, (float3x3)mesh.WorldInverseTranspose));
	float3 tangentWorld = normalize(mul(localTangent, (float3x3)mesh.WorldMatrix));

	if (dot(vertexNormal, outwardGeometricNormal) < 0.0f)
	{
		vertexNormal = -vertexNormal;
	}

	tangentWorld = OrthonormalizeTangent(tangentWorld, vertexNormal);
	float3 bitangentWorld = ComputeBitangentFromSign(vertexNormal, tangentWorld, tangentSign);

	const float2 texCoord0 = InterpolateRayTracingTexCoord0(evaluatedTriangle);
	const RayTracingMaterialSample material =
	    EvaluateRayTracingHitMaterial(evaluatedTriangle.Material, texCoord0, InterpolateRayTracingColor(evaluatedTriangle));

	float3 shadingNormal = TransformTangentNormalToWorld(material.NormalTangent, vertexNormal, tangentWorld, bitangentWorld);
	shadingNormal *= faceSign;
	tangentWorld *= faceSign;
	bitangentWorld *= faceSign;

	if (dot(shadingNormal, geometricNormal) <= 0.0f)
	{
		surface.RejectionReason = RayTracingHitSurface::ReasonInvalidHitData;
		return surface;
	}

	surface.Valid = true;
	surface.PositionWorld = positionWorld;
	surface.PreviousPositionWorld = positionWorld;

	surface.GeometricNormalWorld = geometricNormal;
	surface.PositionError = RayEndpoints::SurfaceErrorBound(evaluatedTriangle, mesh, positionObject, positionWorld, normalObject);
	surface.NormalWorld = shadingNormal;
	surface.TangentWorld = tangentWorld;
	surface.BitangentWorld = bitangentWorld;
	surface.NormalTangent = material.NormalTangent;
	surface.TangentSign = tangentSign;
	surface.TexCoord0 = texCoord0;

	surface.MaterialSlot = evaluatedTriangle.Instance.MaterialSlot;
	surface.GeometryFlags = evaluatedTriangle.Instance.GeometryFlags;
	surface.RejectionReason = RayTracingHitSurface::ReasonNone;

	surface.BaseColor = material.BaseColor.rgb;
	surface.EmissiveColor = material.Emissive;
	surface.SubsurfaceColor = material.SubsurfaceColor;
	surface.Roughness = material.Roughness;
	surface.Metallic = material.Metallic;
	surface.DielectricF0 = evaluatedTriangle.Material.F0;
	surface.AmbientOcclusion = material.AmbientOcclusion;
	surface.Alpha = material.BaseColor.a;
	surface.SubsurfaceStrength = material.SubsurfaceStrength;
	surface.AlphaMode = evaluatedTriangle.Material.AlphaMode;

	surface.GpuSceneSlot = mesh.GpuSceneSlot;
	surface.InstanceId = trace.InstanceId;
	surface.PrimitiveIndex = trace.PrimitiveIndex;
	surface.EmissionTwoSided = twoSided;

	return surface;
}

RayTracingHitSurfaceData ReconstructRayTracingHitSurfaceWithPrevious(RayTracingTraceResult trace, float3 rayDirectionWorld)
{
	RayTracingHitSurfaceData surface = ReconstructRayTracingHitSurface(trace, rayDirectionWorld);
	if (surface.Valid)
	{
		const RayTracingHitTriangle hitTriangle = LoadRayTracingHitTriangle(trace.InstanceId, trace.PrimitiveIndex, trace.Barycentrics);
		const MeshInstanceData mesh = MeshInstances[trace.InstanceId];
		const float3 previousPositionObject = EvaluatePreviousRayTracingPosition(hitTriangle, mesh);

		surface.PreviousPositionWorld = RayEndpoints::TransformPosition(previousPositionObject, mesh.PreviousWorldMatrix);
	}

	return surface;
}
