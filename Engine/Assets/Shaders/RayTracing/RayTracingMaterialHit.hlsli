#pragma once

#include "/Engine/Geometry/Basis.hlsli"
#include "/Engine/Material/MaterialNormal.hlsli"
#include "/Engine/RayTracing/RayEndpoints.hlsli"
#include "/Engine/RayTracing/RayTracingHitSurface.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialAlpha.hlsli"
#include "/Engine/RayTracing/RayTracingTraceResult.hlsli"

void ResolveRayTracingHitMaterialTextures(RayTracingHitMaterial material,
                                          float2 uv,
                                          out float4 baseColor,
                                          out float roughness,
                                          out float metallic,
                                          out float3 emissive,
                                          out float3 normalTangent,
                                          out float ambientOcclusion,
                                          out float3 subsurfaceColor,
                                          out float subsurfaceStrength)
{
	baseColor = material.BaseColor;
	roughness = material.Roughness;
	metallic = material.Metallic;
	emissive = material.EmissiveColor;
	normalTangent = float3(0.0f, 0.0f, 1.0f);
	ambientOcclusion = 1.0f;
	subsurfaceColor = material.SubsurfaceColor;
	subsurfaceStrength = material.SubsurfaceStrength;

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotNormal))
	{
		const MaterialTextureMappingData mapping = material.TextureMappings[MaterialTextureTableSampling::TextureSlotNormal];
		normalTangent = UnpackMaterialNormal(
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotNormal, uv).rgb, mapping.Strength);
	}
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotBaseColor))
	{
		baseColor = SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotBaseColor, uv) * material.BaseColor;
	}
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotRoughness))
	{
		roughness = SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotRoughness, uv).r * material.Roughness;
	}
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotMetallic))
	{
		metallic = SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotMetallic, uv).r * material.Metallic;
	}
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotEmissive))
	{
		emissive = SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotEmissive, uv).rgb
		         * material.EmissiveColor;
	}
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotOcclusion))
	{
		const MaterialTextureMappingData mapping = material.TextureMappings[MaterialTextureTableSampling::TextureSlotOcclusion];
		const float sampledOcclusion =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotOcclusion, uv).r;
		ambientOcclusion = lerp(1.0f, sampledOcclusion, mapping.Strength);
	}
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotSubsurfaceColor))
	{
		subsurfaceColor = SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotSubsurfaceColor, uv).rgb
		                * material.SubsurfaceColor;
	}
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotSubsurfaceStrength))
	{
		subsurfaceStrength =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotSubsurfaceStrength, uv).r
		    * material.SubsurfaceStrength;
	}
}

RayTracingHitSurfaceData ReconstructRayTracingHitSurface(RayTracingTraceResult trace, float3 rayDirectionWorld)
{
	const RayTracingEvaluatedTriangle triangle =
	    EvaluateRayTracingTriangle(trace.InstanceId, trace.PrimitiveIndex, trace.Barycentrics);
	const MeshInstanceData mesh = MeshInstances[trace.InstanceId];
	const float3 positionObject = InterpolateRayTracingPosition(triangle);
	const float3 positionWorld = mul(float4(positionObject, 1.0f), mesh.WorldMatrix).xyz;
	const float3 p0World = mul(float4(triangle.V0.Position, 1.0f), mesh.WorldMatrix).xyz;
	const float3 p1World = mul(float4(triangle.V1.Position, 1.0f), mesh.WorldMatrix).xyz;
	const float3 p2World = mul(float4(triangle.V2.Position, 1.0f), mesh.WorldMatrix).xyz;
	const float3 normalObject = cross(triangle.V1.Position - triangle.V0.Position, triangle.V2.Position - triangle.V0.Position);
	const float3 outwardGeometricNormal = normalize(cross(p1World - p0World, p2World - p0World));
	const bool twoSided = (triangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) != 0u;

	RayTracingHitSurfaceData surface = (RayTracingHitSurfaceData)0;
	if (!trace.FrontFace && !twoSided)
	{
		surface.RejectionReason = RayTracingHitSurface::ReasonOneSidedBackface;
		return surface;
	}

	const float faceSign = trace.FrontFace ? 1.0f : -1.0f;
	const float3 geometricNormal = outwardGeometricNormal * faceSign;
	const float3 localNormal = triangle.V0.Normal * triangle.BarycentricWeights.x
	                         + triangle.V1.Normal * triangle.BarycentricWeights.y
	                         + triangle.V2.Normal * triangle.BarycentricWeights.z;
	const float3 localTangent = triangle.V0.Tangent * triangle.BarycentricWeights.x
	                          + triangle.V1.Tangent * triangle.BarycentricWeights.y
	                          + triangle.V2.Tangent * triangle.BarycentricWeights.z;
	const float tangentSign = triangle.V0.TangentSign * triangle.BarycentricWeights.x
	                        + triangle.V1.TangentSign * triangle.BarycentricWeights.y
	                        + triangle.V2.TangentSign * triangle.BarycentricWeights.z
	                            >= 0.0f
	                        ? 1.0f
	                        : -1.0f;
	float3 vertexNormal = normalize(mul(localNormal, (float3x3)mesh.WorldInverseTranspose));
	float3 tangentWorld = normalize(mul(localTangent, (float3x3)mesh.WorldMatrix));
	if (dot(vertexNormal, outwardGeometricNormal) < 0.0f)
	{
		vertexNormal = -vertexNormal;
	}
	tangentWorld = OrthonormalizeTangent(tangentWorld, vertexNormal);
	float3 bitangentWorld = ComputeBitangentFromSign(vertexNormal, tangentWorld, tangentSign);

	float4 baseColor;
	float roughness;
	float metallic;
	float3 emissive;
	float3 normalTangent;
	float ambientOcclusion;
	float3 subsurfaceColor;
	float subsurfaceStrength;
	const float2 texCoord0 = InterpolateRayTracingTexCoord0(triangle);
	ResolveRayTracingHitMaterialTextures(triangle.Material,
	                                     texCoord0,
	                                     baseColor,
	                                     roughness,
	                                     metallic,
	                                     emissive,
	                                     normalTangent,
	                                     ambientOcclusion,
	                                     subsurfaceColor,
	                                     subsurfaceStrength);
	baseColor *= InterpolateRayTracingColor(triangle);
	float3 shadingNormal = TransformTangentNormalToWorld(normalTangent, vertexNormal, tangentWorld, bitangentWorld);
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
	surface.PositionError = RayEndpoints::SurfaceErrorBound(
	    triangle, mesh, positionObject, positionWorld, normalObject, geometricNormal);
	surface.NormalWorld = shadingNormal;
	surface.TangentWorld = tangentWorld;
	surface.BitangentWorld = bitangentWorld;
	surface.NormalTangent = normalTangent;
	surface.TangentSign = tangentSign;
	surface.TexCoord0 = texCoord0;
	surface.MaterialSlot = triangle.Instance.MaterialSlot;
	surface.GeometryFlags = triangle.Instance.GeometryFlags;
	surface.RejectionReason = RayTracingHitSurface::ReasonNone;
	surface.BaseColor = baseColor.rgb;
	surface.EmissiveColor = emissive;
	surface.SubsurfaceColor = subsurfaceColor;
	surface.Roughness = roughness;
	surface.Metallic = metallic;
	surface.DielectricF0 = triangle.Material.F0;
	surface.AmbientOcclusion = ambientOcclusion;
	surface.Alpha = baseColor.a;
	surface.SubsurfaceStrength = subsurfaceStrength;
	surface.AlphaMode = triangle.Material.AlphaMode;
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
		const RayTracingHitTriangle triangle =
		    LoadRayTracingHitTriangle(trace.InstanceId, trace.PrimitiveIndex, trace.Barycentrics);
		const MeshInstanceData mesh = MeshInstances[trace.InstanceId];
		const float3 previousPositionObject = EvaluatePreviousRayTracingPosition(triangle, mesh);
		surface.PreviousPositionWorld = mul(float4(previousPositionObject, 1.0f), mesh.PreviousWorldMatrix).xyz;
	}
	return surface;
}
