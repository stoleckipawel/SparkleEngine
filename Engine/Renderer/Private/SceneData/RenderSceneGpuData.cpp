#include "PCH.h"
#include "SceneData/RenderSceneGpuData.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "Lighting/LightingCVars.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/RayTracingHitData.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "SceneData/RenderSceneData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/RenderConstantBufferData.h"

#include <algorithm>
#include <cstddef>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	template <typename TValue>
	OwnedStructuredBuffer Upload(
	    RhiResourceService& resourceService,
	    const std::vector<TValue>& values,
	    const wchar_t* debugName)
	{
		return OwnedStructuredBuffer::Upload(resourceService, std::span<const TValue>{values}, debugName);
	}

	template <typename TValue>
	OwnedStructuredBuffer UploadWithEmptySentinel(
	    RhiResourceService& resourceService,
	    std::vector<TValue> values,
	    const wchar_t* debugName)
	{
		if (values.empty())
		{
			values.emplace_back();
		}
		return Upload(resourceService, values, debugName);
	}

	RenderSceneGpuLightingData BuildLightingData(RhiResourceService& resourceService, const RenderSceneData& sceneData)
	{
		const std::size_t directionalLightCount =
		    std::min(sceneData.directionalLights.size(), static_cast<std::size_t>(CVarMaxDirectionalLights.Get()));
		const std::size_t pointLightCount =
		    std::min(sceneData.pointLights.size(), static_cast<std::size_t>(CVarMaxPointLights.Get()));
		const std::size_t spotLightCount =
		    std::min(sceneData.spotLights.size(), static_cast<std::size_t>(CVarMaxSpotLights.Get()));
		const std::size_t rectLightCount =
		    std::min(sceneData.rectLights.size(), static_cast<std::size_t>(CVarMaxRectLights.Get()));

		std::vector<DirectionalLightConstantBufferData> directionalLights;
		std::vector<PointLightConstantBufferData> pointLights;
		std::vector<SpotLightConstantBufferData> spotLights;
		std::vector<RectLightConstantBufferData> rectLights;
		directionalLights.reserve(directionalLightCount);
		pointLights.reserve(pointLightCount);
		spotLights.reserve(spotLightCount);
		rectLights.reserve(rectLightCount);

		for (std::size_t lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
		{
			const auto& light = sceneData.directionalLights[lightIndex];
			directionalLights.push_back(
			    DirectionalLightConstantBufferData{
			        .Direction = {light.direction.x, light.direction.y, light.direction.z},
			        .Intensity = light.intensity,
			        .Color = {light.color.x, light.color.y, light.color.z},
			        .AngularDiameter = light.angularDiameterRadians,
			        .CastShadow = light.castShadow ? 1u : 0u});
		}

		for (std::size_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
		{
			const auto& light = sceneData.pointLights[lightIndex];
			pointLights.push_back(
			    PointLightConstantBufferData{
			        .Position = {light.position.x, light.position.y, light.position.z},
			        .Range = light.range,
			        .Color = {light.color.x, light.color.y, light.color.z},
			        .Intensity = light.intensity,
			        .SourceRadius = light.sourceRadius,
			        .CastShadow = light.castShadow ? 1u : 0u});
		}

		for (std::size_t lightIndex = 0; lightIndex < spotLightCount; ++lightIndex)
		{
			const auto& light = sceneData.spotLights[lightIndex];
			spotLights.push_back(
			    SpotLightConstantBufferData{
			        .Position = {light.position.x, light.position.y, light.position.z},
			        .Range = light.range,
			        .Direction = {light.direction.x, light.direction.y, light.direction.z},
			        .InnerConeCosine = light.innerConeCosine,
			        .Color = {light.color.x, light.color.y, light.color.z},
			        .Intensity = light.intensity,
			        .OuterConeCosine = light.outerConeCosine,
			        .CastShadow = light.castShadow ? 1u : 0u,
			        .SourceRadius = light.sourceRadius});
		}

		for (std::size_t lightIndex = 0; lightIndex < rectLightCount; ++lightIndex)
		{
			const auto& light = sceneData.rectLights[lightIndex];
			rectLights.push_back(
			    RectLightConstantBufferData{
			        .Position = {light.position.x, light.position.y, light.position.z},
			        .Width = light.width,
			        .Direction = {light.direction.x, light.direction.y, light.direction.z},
			        .Height = light.height,
			        .Tangent = {light.tangent.x, light.tangent.y, light.tangent.z},
			        .Luminance = light.luminance,
			        .Color = {light.color.x, light.color.y, light.color.z},
			        .CastShadow = light.castShadow ? 1u : 0u});
		}

		RenderSceneGpuLightingData data{
		    .Constants =
		        ViewLightingData{
		            .DirectionalLightCount = static_cast<std::uint32_t>(directionalLightCount),
		            .PointLightCount = static_cast<std::uint32_t>(pointLightCount),
		            .SpotLightCount = static_cast<std::uint32_t>(spotLightCount),
		            .RectLightCount = static_cast<std::uint32_t>(rectLightCount)},
		    .DirectionalLights = UploadWithEmptySentinel(resourceService, std::move(directionalLights), L"DirectionalLights"),
		    .PointLights = UploadWithEmptySentinel(resourceService, std::move(pointLights), L"PointLights"),
		    .SpotLights = UploadWithEmptySentinel(resourceService, std::move(spotLights), L"SpotLights"),
		    .RectLights = UploadWithEmptySentinel(resourceService, std::move(rectLights), L"RectLights")};
		if (!data.DirectionalLights || !data.PointLights || !data.SpotLights || !data.RectLights)
		{
			return {};
		}
		return data;
	}

	RenderSceneGpuGeometryData BuildGeometryData(RhiResourceService& resourceService, const RenderSceneData& sceneData)
	{
		std::vector<MeshInstanceData> instances;
		instances.reserve((std::max<std::size_t>)(sceneData.meshInstances.size(), 1u));
		for (const MeshDraw& draw : sceneData.meshInstances)
		{
			instances.push_back(
			    MeshInstanceData{
			        .WorldMTX = draw.Transform.WorldMatrix,
			        .PreviousWorldMTX = draw.Transform.PreviousWorldMatrix,
			        .WorldInvTransposeMTX = draw.Transform.WorldInvTranspose,
			        .MaterialSlot = draw.Material.Slot,
			        .Flags = draw.Geometry.MeshKind == RenderMeshKind::Skeletal &&
			                         draw.Skinning.JointMatrixOffset != kInvalidMeshInstanceJointMatrixOffset
			                     ? MeshInstanceFlag_Skinned
			                     : 0u,
			        .JointMatrixOffset = draw.Skinning.JointMatrixOffset,
			        .DebugData = static_cast<std::uint32_t>(instances.size())});
		}

		std::vector<JointMatrixData> jointMatrices;
		std::vector<JointMatrixData> previousJointMatrices;
		jointMatrices.reserve((std::max<std::size_t>)(sceneData.jointMatrices.size(), 1u));
		previousJointMatrices.reserve((std::max<std::size_t>)(sceneData.previousJointMatrices.size(), 1u));
		if (sceneData.jointMatrices.empty())
		{
			jointMatrices.push_back(JointMatrixData{.SkinningMTX = MathUtils::IdentityFloat4x4()});
			previousJointMatrices.push_back(JointMatrixData{.SkinningMTX = MathUtils::IdentityFloat4x4()});
		}
		else
		{
			for (const DirectX::XMFLOAT4X4& matrix : sceneData.jointMatrices)
			{
				jointMatrices.push_back(JointMatrixData{.SkinningMTX = matrix});
			}
			const auto& previousMatrices = sceneData.previousJointMatrices.size() == sceneData.jointMatrices.size()
			                                   ? sceneData.previousJointMatrices
			                                   : sceneData.jointMatrices;
			for (const DirectX::XMFLOAT4X4& matrix : previousMatrices)
			{
				previousJointMatrices.push_back(JointMatrixData{.SkinningMTX = matrix});
			}
		}

		return RenderSceneGpuGeometryData{
		    .MeshInstances = UploadWithEmptySentinel(resourceService, std::move(instances), L"MeshInstances"),
		    .JointMatrices = Upload(resourceService, jointMatrices, L"SkinningJointMatrices"),
		    .PreviousJointMatrices = Upload(resourceService, previousJointMatrices, L"PreviousSkinningJointMatrices")};
	}

	struct MeshHitDataOffsets
	{
		std::uint32_t FirstVertex = 0u;
		std::uint32_t FirstIndex = 0u;
		std::uint32_t VertexCount = 0u;
		std::uint32_t IndexCount = 0u;
	};

	VertexSkinInfluenceData ToHitSkinInfluence(const VertexSkinInfluence& influence) noexcept
	{
		return VertexSkinInfluenceData{
		    .JointIndices = {influence.jointIndices[0], influence.jointIndices[1], influence.jointIndices[2], influence.jointIndices[3]},
		    .JointWeights = {influence.jointWeights[0], influence.jointWeights[1], influence.jointWeights[2], influence.jointWeights[3]}};
	}

	std::uint32_t BuildHitMaterialFlags(const MaterialData& material) noexcept
	{
		std::uint32_t flags = material.doubleSided ? RayTracingHitData::MaterialFlag_DoubleSided : 0u;
		if (material.alphaMode == 0u)
		{
			flags |= RayTracingHitData::MaterialFlag_Opaque;
		}
		else if (material.alphaMode == 1u)
		{
			flags |= RayTracingHitData::MaterialFlag_AlphaTested;
		}
		else if (material.alphaMode == 2u)
		{
			flags |= RayTracingHitData::MaterialFlag_AlphaBlended;
		}
		if (material.textureFlags != 0u)
		{
			flags |= RayTracingHitData::MaterialFlag_Textured;
		}
		return flags;
	}

	std::uint32_t BuildHitGeometryFlags(const MeshDraw& draw, const MaterialData* material) noexcept
	{
		std::uint32_t flags = draw.Geometry.MeshKind == RenderMeshKind::Skeletal ? RayTracingHitData::GeometryFlag_SkinnedMesh
		                                                                      : RayTracingHitData::GeometryFlag_StaticMesh;
		if (material == nullptr)
		{
			return flags;
		}
		if (material->alphaMode == 1u)
		{
			flags |= RayTracingHitData::GeometryFlag_AlphaTested;
		}
		else if (material->alphaMode == 2u)
		{
			flags |= RayTracingHitData::GeometryFlag_AlphaBlended;
		}
		if (material->textureFlags != 0u)
		{
			flags |= RayTracingHitData::GeometryFlag_TexturedMaterial;
		}
		if (material->doubleSided)
		{
			flags |= RayTracingHitData::GeometryFlag_DoubleSided;
		}
		return flags;
	}

	RayTracingHitInstance BuildInvalidHitInstance(
	    const MeshDraw& draw,
	    const MaterialData* material,
	    std::uint32_t rejectionReason) noexcept
	{
		return RayTracingHitInstance{
		    .MaterialSlot = draw.Material.Slot,
		    .GeometryFlags = BuildHitGeometryFlags(draw, material),
		    .RejectionReason = rejectionReason,
		    .AlphaMode = material != nullptr ? material->alphaMode : 0u,
		    .MaterialTextureFlags = material != nullptr ? material->textureFlags : 0u};
	}

	std::uint32_t ValidateMeshHitData(const GPUMesh& gpuMesh) noexcept
	{
		if (!gpuMesh.HasRayTracingHitData())
		{
			return RayTracingHitData::Reason_MissingMeshHitData;
		}

		const std::span<const RayTracingHitVertex> vertices = gpuMesh.GetRayTracingHitVertices();
		const std::span<const std::uint32_t> indices = gpuMesh.GetRayTracingHitIndices();
		if (indices.size() < 3u || indices.size() % 3u != 0u)
		{
			return RayTracingHitData::Reason_InvalidPrimitive;
		}
		for (const std::uint32_t index : indices)
		{
			if (static_cast<std::size_t>(index) >= vertices.size())
			{
				return RayTracingHitData::Reason_InvalidVertexIndex;
			}
		}
		return RayTracingHitData::Reason_None;
	}

	RenderSceneGpuRayTracingData BuildRayTracingData(RhiResourceService& resourceService, const RenderSceneData& sceneData)
	{
		if (sceneData.meshInstances.empty() || sceneData.materials.empty())
		{
			return {};
		}

		std::vector<RayTracingHitVertex> vertices;
		std::vector<VertexSkinInfluenceData> skinInfluences;
		std::vector<std::uint32_t> indices;
		std::vector<RayTracingHitInstance> instances(sceneData.meshInstances.size());
		std::vector<RayTracingHitMaterial> materials;
		materials.reserve(sceneData.materials.size());
		for (const MaterialData& material : sceneData.materials)
		{
			materials.push_back(
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
			        .SubsurfaceStrength = material.subsurfaceStrength,
			        .Flags = BuildHitMaterialFlags(material),
			        .TextureIndices0 =
			            DirectX::XMUINT4{
			                material.materialTextureIndices[MaterialTextureSlots::BaseColor],
			                material.materialTextureIndices[MaterialTextureSlots::Normal],
			                material.materialTextureIndices[MaterialTextureSlots::Roughness],
			                material.materialTextureIndices[MaterialTextureSlots::Metallic]},
			        .TextureIndices1 =
			            DirectX::XMUINT4{
			                material.materialTextureIndices[MaterialTextureSlots::Occlusion],
			                material.materialTextureIndices[MaterialTextureSlots::Emissive],
			                material.materialTextureIndices[MaterialTextureSlots::SubsurfaceColor],
			                material.materialTextureIndices[MaterialTextureSlots::SubsurfaceStrength]}});
		}

		std::unordered_map<const GPUMesh*, MeshHitDataOffsets> meshOffsets;
		std::uint32_t validInstanceCount = 0u;
		for (std::uint32_t instanceIndex = 0u; instanceIndex < static_cast<std::uint32_t>(sceneData.meshInstances.size()); ++instanceIndex)
		{
			const MeshDraw& draw = sceneData.meshInstances[instanceIndex];
			const MaterialData* material = draw.Material.Slot < materials.size() ? &sceneData.materials[draw.Material.Slot] : nullptr;
			if (material == nullptr)
			{
				instances[instanceIndex] = BuildInvalidHitInstance(draw, nullptr, RayTracingHitData::Reason_InvalidMaterial);
				continue;
			}

			const GPUMesh* gpuMesh = draw.Geometry.GpuMesh;
			if (gpuMesh == nullptr ||
			    (draw.Geometry.MeshKind == RenderMeshKind::Skeletal &&
			     (draw.Skinning.JointMatrixOffset == kInvalidMeshInstanceJointMatrixOffset || !gpuMesh->HasSkinInfluences())))
			{
				instances[instanceIndex] = BuildInvalidHitInstance(draw, material, RayTracingHitData::Reason_MissingMeshHitData);
				continue;
			}

			const std::uint32_t validationReason = ValidateMeshHitData(*gpuMesh);
			if (validationReason != RayTracingHitData::Reason_None)
			{
				instances[instanceIndex] = BuildInvalidHitInstance(draw, material, validationReason);
				continue;
			}

			MeshHitDataOffsets offsets{};
			const auto existing = meshOffsets.find(gpuMesh);
			if (existing != meshOffsets.end())
			{
				offsets = existing->second;
			}
			else
			{
				offsets = MeshHitDataOffsets{
				    .FirstVertex = static_cast<std::uint32_t>(vertices.size()),
				    .FirstIndex = static_cast<std::uint32_t>(indices.size()),
				    .VertexCount = static_cast<std::uint32_t>(gpuMesh->GetRayTracingHitVertices().size()),
				    .IndexCount = static_cast<std::uint32_t>(gpuMesh->GetRayTracingHitIndices().size())};
				vertices.insert(vertices.end(), gpuMesh->GetRayTracingHitVertices().begin(), gpuMesh->GetRayTracingHitVertices().end());
				if (gpuMesh->HasSkinInfluences())
				{
					for (const VertexSkinInfluence& influence : gpuMesh->GetSkinInfluences())
					{
						skinInfluences.push_back(ToHitSkinInfluence(influence));
					}
				}
				else
				{
					skinInfluences.resize(vertices.size());
				}
				indices.insert(indices.end(), gpuMesh->GetRayTracingHitIndices().begin(), gpuMesh->GetRayTracingHitIndices().end());
				meshOffsets.emplace(gpuMesh, offsets);
			}

			instances[instanceIndex] = RayTracingHitInstance{
			    .FirstVertex = offsets.FirstVertex,
			    .FirstIndex = offsets.FirstIndex,
			    .VertexCount = offsets.VertexCount,
			    .IndexCount = offsets.IndexCount,
			    .MaterialSlot = draw.Material.Slot,
			    .Flags = RayTracingHitData::InstanceFlag_Valid |
			             (material->alphaMode == 0u ? RayTracingHitData::InstanceFlag_Opaque : 0u) |
			             (draw.Geometry.MeshKind == RenderMeshKind::Static ? RayTracingHitData::InstanceFlag_StaticMesh : 0u) |
			             (material->doubleSided ? RayTracingHitData::InstanceFlag_TwoSided : 0u),
			    .GeometryFlags = BuildHitGeometryFlags(draw, material),
			    .RejectionReason = RayTracingHitData::Reason_None,
			    .AlphaMode = material->alphaMode,
			    .MaterialTextureFlags = material->textureFlags};
			++validInstanceCount;
		}

		if (validInstanceCount == 0u || vertices.empty() || indices.empty() || materials.empty())
		{
			return {};
		}

		RenderSceneGpuRayTracingData data{
		    .Vertices = Upload(resourceService, vertices, L"RayTracingHitVertices"),
		    .SkinInfluences = Upload(resourceService, skinInfluences, L"RayTracingHitSkinInfluences"),
		    .Indices = Upload(resourceService, indices, L"RayTracingHitIndices"),
		    .Instances = Upload(resourceService, instances, L"RayTracingHitInstances"),
		    .Materials = Upload(resourceService, materials, L"RayTracingHitMaterials"),
		    .InstanceCount = static_cast<std::uint32_t>(instances.size()),
		    .MaterialCount = static_cast<std::uint32_t>(materials.size())};
		if (!data.IsValid())
		{
			return {};
		}
		return data;
	}

	template <typename TValue> FrameGraphBufferHandle DeclareBuffer(FrameGraphBuilder& builder, const char* name)
	{
		return builder.ReservePersistentBuffer(
		    FrameGraphBufferDesc::Create(name, sizeof(TValue), static_cast<std::uint32_t>(sizeof(TValue))),
		    ResourceState::ShaderResource);
	}

	void BindBuffer(FrameGraph& graph, FrameGraphBufferHandle handle, const OwnedStructuredBuffer& buffer, const char* name) noexcept
	{
		if (!buffer)
		{
			graph.ClearPersistentBufferBinding(handle);
			return;
		}
		graph.BindPersistentBuffer(
		    handle,
		    buffer.GetResource(),
		    FrameGraphBufferDesc::Create(name, buffer.GetSizeInBytes(), buffer.GetStrideInBytes()),
		    ResourceState::ShaderResource);
	}
}

RenderSceneGpuData BuildRenderSceneGpuData(RhiResourceService& resourceService, const RenderSceneData& sceneData)
{
	return RenderSceneGpuData{
	    .Lighting = BuildLightingData(resourceService, sceneData),
	    .Geometry = BuildGeometryData(resourceService, sceneData),
	    .RayTracing = BuildRayTracingData(resourceService, sceneData)};
}

RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder)
{
	return RenderSceneGpuResources{
	    .Lighting =
	        RenderSceneGpuLightingResources{
	            .DirectionalLights = DeclareBuffer<DirectionalLightConstantBufferData>(builder, "DirectionalLights"),
	            .PointLights = DeclareBuffer<PointLightConstantBufferData>(builder, "PointLights"),
	            .SpotLights = DeclareBuffer<SpotLightConstantBufferData>(builder, "SpotLights"),
	            .RectLights = DeclareBuffer<RectLightConstantBufferData>(builder, "RectLights")},
	    .Geometry =
	        RenderSceneGpuGeometryResources{
	            .MeshInstances = DeclareBuffer<MeshInstanceData>(builder, "MeshInstances"),
	            .JointMatrices = DeclareBuffer<JointMatrixData>(builder, "JointMatrices"),
	            .PreviousJointMatrices = DeclareBuffer<JointMatrixData>(builder, "PreviousJointMatrices")},
	    .RayTracing =
	        RenderSceneGpuRayTracingResources{
	            .Vertices = DeclareBuffer<RayTracingHitVertex>(builder, "RayTracingHitVertices"),
	            .SkinInfluences = DeclareBuffer<VertexSkinInfluenceData>(builder, "RayTracingHitSkinInfluences"),
	            .Indices = DeclareBuffer<std::uint32_t>(builder, "RayTracingHitIndices"),
	            .Instances = DeclareBuffer<RayTracingHitInstance>(builder, "RayTracingHitInstances"),
	            .Materials = DeclareBuffer<RayTracingHitMaterial>(builder, "RayTracingHitMaterials")}};
}

void BindRenderSceneGpuResources(
    FrameGraph& graph,
    const RenderSceneGpuResources& resources,
    const RenderSceneGpuData& sceneGpuData) noexcept
{
	BindBuffer(graph, resources.Lighting.DirectionalLights, sceneGpuData.Lighting.DirectionalLights, "DirectionalLights");
	BindBuffer(graph, resources.Lighting.PointLights, sceneGpuData.Lighting.PointLights, "PointLights");
	BindBuffer(graph, resources.Lighting.SpotLights, sceneGpuData.Lighting.SpotLights, "SpotLights");
	BindBuffer(graph, resources.Lighting.RectLights, sceneGpuData.Lighting.RectLights, "RectLights");
	BindBuffer(graph, resources.Geometry.MeshInstances, sceneGpuData.Geometry.MeshInstances, "MeshInstances");
	BindBuffer(graph, resources.Geometry.JointMatrices, sceneGpuData.Geometry.JointMatrices, "JointMatrices");
	BindBuffer(
	    graph,
	    resources.Geometry.PreviousJointMatrices,
	    sceneGpuData.Geometry.PreviousJointMatrices,
	    "PreviousJointMatrices");
	BindBuffer(graph, resources.RayTracing.Vertices, sceneGpuData.RayTracing.Vertices, "RayTracingHitVertices");
	BindBuffer(
	    graph,
	    resources.RayTracing.SkinInfluences,
	    sceneGpuData.RayTracing.SkinInfluences,
	    "RayTracingHitSkinInfluences");
	BindBuffer(graph, resources.RayTracing.Indices, sceneGpuData.RayTracing.Indices, "RayTracingHitIndices");
	BindBuffer(graph, resources.RayTracing.Instances, sceneGpuData.RayTracing.Instances, "RayTracingHitInstances");
	BindBuffer(graph, resources.RayTracing.Materials, sceneGpuData.RayTracing.Materials, "RayTracingHitMaterials");
}
