#include "PCH.h"
#include "SceneData/GpuScene/RenderGpuScenePayloadBuilder.h"

#include "Core/Public/Math/MathUtils.h"
#include "Lighting/LightingCVars.h"
#include "Meshes/GPUMesh.h"
#include "Meshes/GPUMeshCache.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <unordered_map>

struct RenderGpuMeshHitDataOffsets final
{
	std::uint32_t FirstVertex = 0;
	std::uint32_t FirstIndex = 0;
	std::uint32_t FirstMorphTargetDelta = 0;
	std::uint32_t VertexCount = 0;
	std::uint32_t IndexCount = 0;
};

RenderGpuLightingPayloads RenderGpuScenePayloadBuilder::BuildLighting(
    const RenderSceneData& sceneData)
{
	const std::size_t directionalLightCount =
	    std::min(
	        sceneData.directionalLights.size(),
	        static_cast<std::size_t>(CVarMaxDirectionalLights.Get()));
	const std::size_t pointLightCount =
	    std::min(
	        sceneData.pointLights.size(),
	        static_cast<std::size_t>(CVarMaxPointLights.Get()));
	const std::size_t spotLightCount =
	    std::min(
	        sceneData.spotLights.size(),
	        static_cast<std::size_t>(CVarMaxSpotLights.Get()));
	const std::size_t rectLightCount =
	    std::min(
	        sceneData.rectLights.size(),
	        static_cast<std::size_t>(CVarMaxRectLights.Get()));

	RenderGpuLightingPayloads payloads;
	payloads.Constants = ViewLightingData{
	    .DirectionalLightCount =
	        static_cast<std::uint32_t>(directionalLightCount),
	    .PointLightCount =
	        static_cast<std::uint32_t>(pointLightCount),
	    .SpotLightCount =
	        static_cast<std::uint32_t>(spotLightCount),
	    .RectLightCount =
	        static_cast<std::uint32_t>(rectLightCount)};
	payloads.DirectionalLights.reserve(
	    std::max<std::size_t>(directionalLightCount, 1));
	payloads.PointLights.reserve(
	    std::max<std::size_t>(pointLightCount, 1));
	payloads.SpotLights.reserve(
	    std::max<std::size_t>(spotLightCount, 1));
	payloads.RectLights.reserve(
	    std::max<std::size_t>(rectLightCount, 1));

	for (std::size_t lightIndex = 0;
	     lightIndex < directionalLightCount;
	     ++lightIndex)
	{
		const DirectionalLight& light =
		    sceneData.directionalLights[lightIndex];
		payloads.DirectionalLights.push_back(
		    DirectionalLightConstantBufferData{
		        .Direction =
		            {light.direction.x,
		             light.direction.y,
		             light.direction.z},
		        .Intensity = light.intensity,
		        .Color =
		            {light.color.x, light.color.y, light.color.z},
		        .AngularDiameter = light.angularDiameterRadians,
		        .CastShadow = light.castShadow ? 1u : 0u});
	}

	for (std::size_t lightIndex = 0;
	     lightIndex < pointLightCount;
	     ++lightIndex)
	{
		const PointLight& light =
		    sceneData.pointLights[lightIndex];
		payloads.PointLights.push_back(
		    PointLightConstantBufferData{
		        .Position =
		            {light.position.x,
		             light.position.y,
		             light.position.z},
		        .Range = light.range,
		        .Color =
		            {light.color.x, light.color.y, light.color.z},
		        .Intensity = light.intensity,
		        .SourceRadius = light.sourceRadius,
		        .CastShadow = light.castShadow ? 1u : 0u});
	}

	for (std::size_t lightIndex = 0;
	     lightIndex < spotLightCount;
	     ++lightIndex)
	{
		const SpotLight& light =
		    sceneData.spotLights[lightIndex];
		payloads.SpotLights.push_back(
		    SpotLightConstantBufferData{
		        .Position =
		            {light.position.x,
		             light.position.y,
		             light.position.z},
		        .Range = light.range,
		        .Direction =
		            {light.direction.x,
		             light.direction.y,
		             light.direction.z},
		        .InnerConeCosine = light.innerConeCosine,
		        .Color =
		            {light.color.x, light.color.y, light.color.z},
		        .Intensity = light.intensity,
		        .OuterConeCosine = light.outerConeCosine,
		        .CastShadow = light.castShadow ? 1u : 0u,
		        .SourceRadius = light.sourceRadius});
	}

	for (std::size_t lightIndex = 0;
	     lightIndex < rectLightCount;
	     ++lightIndex)
	{
		const RectLight& light =
		    sceneData.rectLights[lightIndex];
		payloads.RectLights.push_back(
		    RectLightConstantBufferData{
		        .Position =
		            {light.position.x,
		             light.position.y,
		             light.position.z},
		        .Width = light.width,
		        .Direction =
		            {light.direction.x,
		             light.direction.y,
		             light.direction.z},
		        .Height = light.height,
		        .Tangent =
		            {light.tangent.x,
		             light.tangent.y,
		             light.tangent.z},
		        .Luminance = light.luminance,
		        .Color =
		            {light.color.x, light.color.y, light.color.z},
		        .CastShadow = light.castShadow ? 1u : 0u});
	}

	if (payloads.DirectionalLights.empty())
	{
		payloads.DirectionalLights.emplace_back();
	}
	if (payloads.PointLights.empty())
	{
		payloads.PointLights.emplace_back();
	}
	if (payloads.SpotLights.empty())
	{
		payloads.SpotLights.emplace_back();
	}
	if (payloads.RectLights.empty())
	{
		payloads.RectLights.emplace_back();
	}
	return payloads;
}

RenderGpuGeometryPayloads RenderGpuScenePayloadBuilder::BuildGeometry(
    const RenderSceneData& sceneData)
{
	RenderGpuGeometryPayloads payloads;
	std::uint32_t meshInstanceCapacity = 1;
	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		meshInstanceCapacity =
		    std::max(
		        meshInstanceCapacity,
		        draw.Source.GpuSceneSlot + 1u);
	}
	payloads.MeshInstances.resize(meshInstanceCapacity);
	payloads.MeshInstanceSlots.reserve(
	    std::max<std::size_t>(
	        sceneData.rasterMeshInstanceIndices.size(),
	        1));
	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		payloads.MeshInstances[draw.Source.GpuSceneSlot] =
		    MeshInstanceData{
		        .WorldMTX = draw.Transform.WorldMatrix,
		        .PreviousWorldMTX =
		            draw.Transform.PreviousWorldMatrix,
		        .WorldInvTransposeMTX =
		            draw.Transform.WorldInvTranspose,
		        .MaterialSlot = draw.Material.Slot,
		        .Flags =
		            (draw.Geometry.MeshKind ==
		                        RenderMeshKind::Skeletal &&
		                    draw.Skinning.JointMatrixOffset !=
		                        kInvalidMeshInstanceJointMatrixOffset
		                ? MeshInstanceFlag_Skinned
		                : 0u) |
		            (draw.Morph.TargetCount > 0u &&
		                     draw.Morph.WeightOffset !=
		                         kInvalidMeshInstanceMorphWeightOffset
		                 ? MeshInstanceFlag_Morphed
		                 : 0u),
		        .JointMatrixOffset =
		            draw.Skinning.JointMatrixOffset,
		        .MorphWeightOffset =
		            draw.Morph.WeightOffset,
		        .MorphTargetCount =
		            draw.Morph.TargetCount,
		        .MorphTargetVertexCount =
		            draw.Morph.VertexCount,
		        .DebugData = draw.Source.GpuSceneSlot};
	}
	for (const std::uint32_t drawIndex :
	     sceneData.rasterMeshInstanceIndices)
	{
		if (drawIndex < sceneData.meshInstances.size())
		{
			payloads.MeshInstanceSlots.push_back(
			    sceneData.meshInstances[drawIndex]
			        .Source.GpuSceneSlot);
		}
	}
	if (payloads.MeshInstanceSlots.empty())
	{
		payloads.MeshInstanceSlots.push_back(0);
	}

	payloads.JointMatrices.reserve(
	    std::max<std::size_t>(sceneData.jointMatrices.size(), 1));
	payloads.PreviousJointMatrices.reserve(
	    std::max<std::size_t>(
	        sceneData.previousJointMatrices.size(),
	        1));
	if (sceneData.jointMatrices.empty())
	{
		const JointMatrixData identity{
		    .SkinningMTX = MathUtils::IdentityFloat4x4()};
		payloads.JointMatrices.push_back(identity);
		payloads.PreviousJointMatrices.push_back(identity);
	}
	else
	{
		for (const DirectX::XMFLOAT4X4& matrix :
		     sceneData.jointMatrices)
		{
			payloads.JointMatrices.push_back(
			    JointMatrixData{.SkinningMTX = matrix});
		}
		const std::vector<DirectX::XMFLOAT4X4>&
		    previousMatrices =
		        sceneData.previousJointMatrices.size() ==
		                sceneData.jointMatrices.size()
		            ? sceneData.previousJointMatrices
		            : sceneData.jointMatrices;
		for (const DirectX::XMFLOAT4X4& matrix :
		     previousMatrices)
		{
			payloads.PreviousJointMatrices.push_back(
			    JointMatrixData{.SkinningMTX = matrix});
		}
	}

	payloads.MorphWeights = sceneData.morphWeights;
	payloads.PreviousMorphWeights =
	    sceneData.previousMorphWeights.size() ==
	            sceneData.morphWeights.size()
	        ? sceneData.previousMorphWeights
	        : sceneData.morphWeights;
	if (payloads.MorphWeights.empty())
	{
		payloads.MorphWeights.push_back(0.0f);
		payloads.PreviousMorphWeights.push_back(0.0f);
	}
	return payloads;
}

RenderGpuRayTracingPayloads
RenderGpuScenePayloadBuilder::BuildRayTracing(
    const RenderSceneData& sceneData,
	const GPUMeshCache& meshes)
{
	RenderGpuRayTracingPayloads payloads;
	const RenderRayTracingWorkPlan& work =
	    sceneData.rayTracingWork;
	if (work.BlasInputs.empty() ||
	    sceneData.materials.empty())
	{
		return payloads;
	}

	payloads.Materials.reserve(sceneData.materials.size());
	for (const MaterialData& material : sceneData.materials)
	{
		payloads.Materials.push_back(
		    RayTracingHitMaterial{
		        .BaseColor = material.baseColor,
		        .EmissiveColor = material.emissiveColor,
		        .Metallic = material.metallic,
		        .Roughness = material.roughness,
		        .F0 = material.f0,
		        .AlphaCutoff = material.alphaCutoff,
		        .AlphaMode = material.alphaMode,
		        .TextureFlags = material.textureFlags,
		        .SubsurfaceColor = material.subsurfaceColor,
		        .SubsurfaceStrength =
		            material.subsurfaceStrength,
		        .Flags = BuildMaterialFlags(material),
		        .TextureIndices0 =
		            DirectX::XMUINT4{
		                material.materialTextureIndices
		                    [MaterialTextureSlots::BaseColor],
		                material.materialTextureIndices
		                    [MaterialTextureSlots::Normal],
		                material.materialTextureIndices
		                    [MaterialTextureSlots::Roughness],
		                material.materialTextureIndices
		                    [MaterialTextureSlots::Metallic]},
		        .TextureIndices1 =
		            DirectX::XMUINT4{
		                material.materialTextureIndices
		                    [MaterialTextureSlots::Occlusion],
		                material.materialTextureIndices
		                    [MaterialTextureSlots::Emissive],
		                material.materialTextureIndices
		                    [MaterialTextureSlots::
		                         SubsurfaceColor],
		                material.materialTextureIndices
		                    [MaterialTextureSlots::
		                         SubsurfaceStrength]}});
	}

	std::uint32_t instanceCapacity = 1;
	for (const RenderRayTracingBlasInput& input :
	     work.BlasInputs)
	{
		instanceCapacity =
		    std::max(
		        instanceCapacity,
		        input.GpuSceneSlot + 1u);
	}
	payloads.Instances.resize(instanceCapacity);

	std::unordered_map<const GPUMesh*, RenderGpuMeshHitDataOffsets>
	    meshOffsets;
	std::uint32_t validInstanceCount = 0;
	for (const RenderRayTracingBlasInput& input :
	     work.BlasInputs)
	{
		if (input.MeshInstanceIndex >=
		    sceneData.meshInstances.size())
		{
			continue;
		}
		const MeshDraw& draw =
		    sceneData.meshInstances[
		        input.MeshInstanceIndex];
		const MaterialData* material =
		    draw.Material.Slot < sceneData.materials.size()
		        ? &sceneData.materials[draw.Material.Slot]
		        : nullptr;
		RayTracingHitInstance& instance =
		    payloads.Instances[input.GpuSceneSlot];
		if (material == nullptr)
		{
			instance = BuildRejectedInstance(
			    draw,
			    nullptr,
			    RayTracingHitData::Reason_InvalidMaterial);
			continue;
		}

		const GPUMesh* gpuMesh =
		    meshes.Resolve(draw.Geometry.Mesh);
		const bool missingSkinning =
		    draw.Geometry.MeshKind == RenderMeshKind::Skeletal &&
		    (draw.Skinning.JointMatrixOffset ==
		         kInvalidMeshInstanceJointMatrixOffset ||
		     gpuMesh == nullptr ||
		     !gpuMesh->HasSkinInfluences());
		if (gpuMesh == nullptr ||
		    !gpuMesh->HasRayTracingHitData() ||
		    missingSkinning)
		{
			instance = BuildRejectedInstance(
			    draw,
			    material,
			    RayTracingHitData::Reason_MissingMeshHitData);
			continue;
		}

		const std::span<const RayTracingHitVertex> meshVertices =
		    gpuMesh->GetRayTracingHitVertices();
		const std::span<const std::uint32_t> meshIndices =
		    gpuMesh->GetRayTracingHitIndices();
		if (meshVertices.empty() || meshIndices.size() < 3)
		{
			instance = BuildRejectedInstance(
			    draw,
			    material,
			    RayTracingHitData::Reason_InvalidPrimitive);
			continue;
		}

		RenderGpuMeshHitDataOffsets offsets;
		const auto existing = meshOffsets.find(gpuMesh);
		if (existing != meshOffsets.end())
		{
			offsets = existing->second;
		}
		else
		{
			offsets = RenderGpuMeshHitDataOffsets{
			    .FirstVertex =
			        static_cast<std::uint32_t>(
			            payloads.Vertices.size()),
			    .FirstIndex =
			        static_cast<std::uint32_t>(
			            payloads.Indices.size()),
			    .FirstMorphTargetDelta =
			        static_cast<std::uint32_t>(
			            payloads.MorphTargetDeltas.size()),
			    .VertexCount =
			        static_cast<std::uint32_t>(
			            meshVertices.size()),
			    .IndexCount =
			        static_cast<std::uint32_t>(
			            meshIndices.size())};
			payloads.Vertices.insert(
			    payloads.Vertices.end(),
			    meshVertices.begin(),
			    meshVertices.end());
			if (gpuMesh->HasSkinInfluences())
			{
				for (const VertexSkinInfluence& influence :
				     gpuMesh->GetSkinInfluences())
				{
					payloads.SkinInfluences.push_back(
					    ConvertSkinInfluence(influence));
				}
			}
			else
			{
				payloads.SkinInfluences.resize(
				    payloads.Vertices.size());
			}
			payloads.Indices.insert(
			    payloads.Indices.end(),
			    meshIndices.begin(),
			    meshIndices.end());
			const std::span<const MorphTargetDeltaData>
			    morphTargetDeltas =
			        gpuMesh->GetMorphTargetDeltas();
			payloads.MorphTargetDeltas.insert(
			    payloads.MorphTargetDeltas.end(),
			    morphTargetDeltas.begin(),
			    morphTargetDeltas.end());
			meshOffsets.emplace(gpuMesh, offsets);
		}

		instance = RayTracingHitInstance{
		    .FirstVertex = offsets.FirstVertex,
		    .FirstIndex = offsets.FirstIndex,
		    .VertexCount = offsets.VertexCount,
		    .IndexCount = offsets.IndexCount,
		    .MaterialSlot = draw.Material.Slot,
		    .Flags =
		        RayTracingHitData::InstanceFlag_Valid |
		        (material->alphaMode == 0
		             ? RayTracingHitData::InstanceFlag_Opaque
		             : 0u) |
		        (draw.Geometry.MeshKind ==
		                 RenderMeshKind::Static
		             ? RayTracingHitData::
		                   InstanceFlag_StaticMesh
		             : 0u) |
		        (material->doubleSided
		             ? RayTracingHitData::
		                   InstanceFlag_TwoSided
		             : 0u),
		    .GeometryFlags =
		        BuildGeometryFlags(draw, material),
		    .RejectionReason = RayTracingHitData::Reason_None,
		    .AlphaMode = material->alphaMode,
		    .MaterialTextureFlags = material->textureFlags};
		instance.MorphTargetDeltaOffset =
		    offsets.FirstMorphTargetDelta;
		++validInstanceCount;
	}

	if (validInstanceCount == 0 || payloads.Vertices.empty() ||
	    payloads.Indices.empty() || payloads.Materials.empty())
	{
		return {};
	}
	payloads.InstanceCount =
	    static_cast<std::uint32_t>(payloads.Instances.size());
	payloads.MaterialCount =
	    static_cast<std::uint32_t>(payloads.Materials.size());
	if (payloads.MorphTargetDeltas.empty())
	{
		payloads.MorphTargetDeltas.emplace_back();
	}
	return payloads;
}

VertexSkinInfluenceData
RenderGpuScenePayloadBuilder::ConvertSkinInfluence(
    const VertexSkinInfluence& influence) noexcept
{
	return VertexSkinInfluenceData{
	    .JointIndices =
	        {influence.jointIndices[0],
	         influence.jointIndices[1],
	         influence.jointIndices[2],
	         influence.jointIndices[3]},
	    .JointWeights =
	        {influence.jointWeights[0],
	         influence.jointWeights[1],
	         influence.jointWeights[2],
	         influence.jointWeights[3]}};
}

std::uint32_t RenderGpuScenePayloadBuilder::BuildMaterialFlags(
    const MaterialData& material) noexcept
{
	std::uint32_t flags =
	    material.doubleSided
	        ? RayTracingHitData::MaterialFlag_DoubleSided
	        : 0u;
	if (material.alphaMode == 0)
	{
		flags |= RayTracingHitData::MaterialFlag_Opaque;
	}
	else if (material.alphaMode == 1)
	{
		flags |= RayTracingHitData::MaterialFlag_AlphaTested;
	}
	else if (material.alphaMode == 2)
	{
		flags |= RayTracingHitData::MaterialFlag_AlphaBlended;
	}
	if (material.textureFlags != 0)
	{
		flags |= RayTracingHitData::MaterialFlag_Textured;
	}
	return flags;
}

std::uint32_t RenderGpuScenePayloadBuilder::BuildGeometryFlags(
    const MeshDraw& draw,
    const MaterialData* material) noexcept
{
	std::uint32_t flags =
	    draw.Geometry.MeshKind == RenderMeshKind::Skeletal
	        ? RayTracingHitData::GeometryFlag_SkinnedMesh
	        : RayTracingHitData::GeometryFlag_StaticMesh;
	if (material == nullptr)
	{
		return flags;
	}
	if (material->alphaMode == 1)
	{
		flags |= RayTracingHitData::GeometryFlag_AlphaTested;
	}
	else if (material->alphaMode == 2)
	{
		flags |= RayTracingHitData::GeometryFlag_AlphaBlended;
	}
	if (material->textureFlags != 0)
	{
		flags |= RayTracingHitData::GeometryFlag_TexturedMaterial;
	}
	if (material->doubleSided)
	{
		flags |= RayTracingHitData::GeometryFlag_DoubleSided;
	}
	return flags;
}

RayTracingHitInstance
RenderGpuScenePayloadBuilder::BuildRejectedInstance(
    const MeshDraw& draw,
    const MaterialData* material,
    std::uint32_t rejectionReason) noexcept
{
	return RayTracingHitInstance{
	    .MaterialSlot = draw.Material.Slot,
	    .GeometryFlags = BuildGeometryFlags(draw, material),
	    .RejectionReason = rejectionReason,
	    .AlphaMode =
	        material != nullptr ? material->alphaMode : 0u,
	    .MaterialTextureFlags =
	        material != nullptr ? material->textureFlags : 0u};
}
