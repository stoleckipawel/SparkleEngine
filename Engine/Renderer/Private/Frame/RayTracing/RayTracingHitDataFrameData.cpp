#include "../../PCH.h"
#include "Frame/RayTracing/RayTracingHitDataFrameData.h"

#include "Meshes/GPUMesh.h"
#include "SceneData/RenderSceneData.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	struct MeshHitDataOffsets
	{
		std::uint32_t FirstVertex = 0u;
		std::uint32_t FirstIndex = 0u;
		std::uint32_t VertexCount = 0u;
		std::uint32_t IndexCount = 0u;
	};

	std::uint32_t BuildHitMaterialFlags(const MaterialData& material) noexcept
	{
		std::uint32_t flags = 0u;
		if (material.doubleSided)
		{
			flags |= RayTracingHitData::MaterialFlag_DoubleSided;
		}
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
		std::uint32_t flags = 0u;
		if (draw.Geometry.MeshKind == RenderMeshKind::Skeletal)
		{
			flags |= RayTracingHitData::GeometryFlag_SkinnedMesh;
		}
		else
		{
			flags |= RayTracingHitData::GeometryFlag_StaticMesh;
		}
		if (material != nullptr)
		{
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

	template <typename TData>
	bool UploadStructuredBuffer(
	    RenderHardwareInterface& renderHardwareInterface,
	    const std::vector<TData>& data,
	    const wchar_t* debugName,
	    RhiOwnedResourceHandle& outBuffer,
	    RhiResourceViewHandle& outView,
	    RhiGpuDescriptorHandle& outShaderResourceView) noexcept
	{
		outBuffer = {};
		outView = {};
		outShaderResourceView = {};
		if (data.empty())
		{
			return false;
		}

		const bool created = renderHardwareInterface.GetResourceService().CreateStructuredBuffer(
		    data.data(),
		    data.size() * sizeof(TData),
		    static_cast<std::uint32_t>(sizeof(TData)),
		    debugName,
		    outBuffer,
		    outView);
		if (!created || !outBuffer || !outView)
		{
			return false;
		}

		outShaderResourceView = renderHardwareInterface.GetDescriptorService().GetResourceViewGpuHandle(outView);
		return static_cast<bool>(outShaderResourceView);
	}
}

RayTracingHitDataFrameData::~RayTracingHitDataFrameData() noexcept
{
	Release();
}

RayTracingHitDataFrameData::RayTracingHitDataFrameData(RayTracingHitDataFrameData&& other) noexcept
{
	*this = std::move(other);
}

RayTracingHitDataFrameData& RayTracingHitDataFrameData::operator=(RayTracingHitDataFrameData&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Release();
	m_renderHardwareInterface = other.m_renderHardwareInterface;
	m_vertexBuffer = other.m_vertexBuffer;
	m_indexBuffer = other.m_indexBuffer;
	m_instanceBuffer = other.m_instanceBuffer;
	m_materialBuffer = other.m_materialBuffer;
	m_vertexView = other.m_vertexView;
	m_indexView = other.m_indexView;
	m_instanceView = other.m_instanceView;
	m_materialView = other.m_materialView;
	m_vertexShaderResourceView = other.m_vertexShaderResourceView;
	m_indexShaderResourceView = other.m_indexShaderResourceView;
	m_instanceShaderResourceView = other.m_instanceShaderResourceView;
	m_materialShaderResourceView = other.m_materialShaderResourceView;
	m_instanceCount = other.m_instanceCount;
	m_materialCount = other.m_materialCount;

	other.m_renderHardwareInterface = nullptr;
	other.m_vertexBuffer = {};
	other.m_indexBuffer = {};
	other.m_instanceBuffer = {};
	other.m_materialBuffer = {};
	other.m_vertexView = {};
	other.m_indexView = {};
	other.m_instanceView = {};
	other.m_materialView = {};
	other.m_vertexShaderResourceView = {};
	other.m_indexShaderResourceView = {};
	other.m_instanceShaderResourceView = {};
	other.m_materialShaderResourceView = {};
	other.m_instanceCount = 0u;
	other.m_materialCount = 0u;
	return *this;
}

bool RayTracingHitDataFrameData::IsValid() const noexcept
{
	return static_cast<bool>(m_vertexShaderResourceView) && static_cast<bool>(m_indexShaderResourceView) &&
	       static_cast<bool>(m_instanceShaderResourceView) && static_cast<bool>(m_materialShaderResourceView) && m_instanceCount > 0u &&
	       m_materialCount > 0u;
}

RayTracingHitDataFrameData RayTracingHitDataFrameData::Build(
    RenderHardwareInterface& renderHardwareInterface,
    const RenderSceneData& sceneData)
{
	if (sceneData.meshInstances.empty() || sceneData.materials.empty())
	{
		return {};
	}

	std::vector<RayTracingHitVertex> vertices;
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
		if (draw.Material.Slot >= materials.size() || material == nullptr)
		{
			instances[instanceIndex] =
			    BuildInvalidHitInstance(draw, nullptr, RayTracingHitData::Reason_InvalidMaterial);
			continue;
		}
		if (draw.Geometry.MeshKind == RenderMeshKind::Skeletal)
		{
			instances[instanceIndex] =
			    BuildInvalidHitInstance(draw, material, RayTracingHitData::Reason_UnsupportedSkinned);
			continue;
		}
		if (material->alphaMode == 2u)
		{
			instances[instanceIndex] =
			    BuildInvalidHitInstance(draw, material, RayTracingHitData::Reason_UnsupportedAlphaMode);
			continue;
		}

		const GPUMesh* gpuMesh = draw.Geometry.GpuMesh;
		if (gpuMesh == nullptr)
		{
			instances[instanceIndex] =
			    BuildInvalidHitInstance(draw, material, RayTracingHitData::Reason_MissingMeshHitData);
			continue;
		}

		const std::uint32_t meshValidationReason = ValidateMeshHitData(*gpuMesh);
		if (meshValidationReason != RayTracingHitData::Reason_None)
		{
			instances[instanceIndex] = BuildInvalidHitInstance(draw, material, meshValidationReason);
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
			vertices.insert(
			    vertices.end(),
			    gpuMesh->GetRayTracingHitVertices().begin(),
			    gpuMesh->GetRayTracingHitVertices().end());
			indices.insert(
			    indices.end(),
			    gpuMesh->GetRayTracingHitIndices().begin(),
			    gpuMesh->GetRayTracingHitIndices().end());
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
		             RayTracingHitData::InstanceFlag_StaticMesh |
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

	RayTracingHitDataFrameData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_instanceCount = static_cast<std::uint32_t>(instances.size());
	frameData.m_materialCount = static_cast<std::uint32_t>(materials.size());
	if (!UploadStructuredBuffer(
	        renderHardwareInterface,
	        vertices,
	        L"RayTracingHitVertices",
	        frameData.m_vertexBuffer,
	        frameData.m_vertexView,
	        frameData.m_vertexShaderResourceView) ||
	    !UploadStructuredBuffer(
	        renderHardwareInterface,
	        indices,
	        L"RayTracingHitIndices",
	        frameData.m_indexBuffer,
	        frameData.m_indexView,
	        frameData.m_indexShaderResourceView) ||
	    !UploadStructuredBuffer(
	        renderHardwareInterface,
	        instances,
	        L"RayTracingHitInstances",
	        frameData.m_instanceBuffer,
	        frameData.m_instanceView,
	        frameData.m_instanceShaderResourceView) ||
	    !UploadStructuredBuffer(
	        renderHardwareInterface,
	        materials,
	        L"RayTracingHitMaterials",
	        frameData.m_materialBuffer,
	        frameData.m_materialView,
	        frameData.m_materialShaderResourceView))
	{
		frameData.Release();
		return {};
	}

	return frameData;
}

void RayTracingHitDataFrameData::Release() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_vertexView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_vertexView);
		}
		if (m_indexView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_indexView);
		}
		if (m_instanceView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_instanceView);
		}
		if (m_materialView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_materialView);
		}
		if (m_vertexBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_vertexBuffer);
		}
		if (m_indexBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_indexBuffer);
		}
		if (m_instanceBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_instanceBuffer);
		}
		if (m_materialBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_materialBuffer);
		}
	}

	m_renderHardwareInterface = nullptr;
	m_vertexBuffer = {};
	m_indexBuffer = {};
	m_instanceBuffer = {};
	m_materialBuffer = {};
	m_vertexView = {};
	m_indexView = {};
	m_instanceView = {};
	m_materialView = {};
	m_vertexShaderResourceView = {};
	m_indexShaderResourceView = {};
	m_instanceShaderResourceView = {};
	m_materialShaderResourceView = {};
	m_instanceCount = 0u;
	m_materialCount = 0u;
}
