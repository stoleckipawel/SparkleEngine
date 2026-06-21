#include "../PCH.h"
#include "Frame/RTIndirectSpecularHitDataFrameData.h"

#include "Meshes/GPUMesh.h"
#include "SceneData/RenderSceneData.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <unordered_map>
#include <utility>
#include <vector>

static const auto g_rtIndirectSpecularHitDataLogger = Logging::GetOrCreateLogger("Renderer.RTIndirectSpecular");

namespace
{
	struct MeshHitDataOffsets
	{
		std::uint32_t FirstVertex = 0u;
		std::uint32_t FirstIndex = 0u;
		std::uint32_t VertexCount = 0u;
		std::uint32_t IndexCount = 0u;
	};

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

RTIndirectSpecularHitDataFrameData::~RTIndirectSpecularHitDataFrameData() noexcept
{
	Release();
}

RTIndirectSpecularHitDataFrameData::RTIndirectSpecularHitDataFrameData(RTIndirectSpecularHitDataFrameData&& other) noexcept
{
	*this = std::move(other);
}

RTIndirectSpecularHitDataFrameData& RTIndirectSpecularHitDataFrameData::operator=(RTIndirectSpecularHitDataFrameData&& other) noexcept
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

bool RTIndirectSpecularHitDataFrameData::IsValid() const noexcept
{
	return static_cast<bool>(m_vertexShaderResourceView) && static_cast<bool>(m_indexShaderResourceView) &&
	       static_cast<bool>(m_instanceShaderResourceView) && static_cast<bool>(m_materialShaderResourceView) && m_instanceCount > 0u &&
	       m_materialCount > 0u;
}

RTIndirectSpecularHitDataFrameData RTIndirectSpecularHitDataFrameData::Build(
    RenderHardwareInterface& renderHardwareInterface,
    const RenderSceneData& sceneData)
{
	if (sceneData.meshInstances.empty() || sceneData.materials.empty())
	{
		return {};
	}

	std::vector<RTIndirectSpecularHitVertex> vertices;
	std::vector<std::uint32_t> indices;
	std::vector<RTIndirectSpecularHitInstance> instances(sceneData.meshInstances.size());
	std::vector<RTIndirectSpecularHitMaterial> materials;
	materials.reserve(sceneData.materials.size());
	for (const MaterialData& material : sceneData.materials)
	{
		materials.push_back(
		    RTIndirectSpecularHitMaterial{
		        .BaseColor = material.baseColor,
		        .EmissiveColor = material.emissiveColor,
		        .Metallic = material.metallic,
		        .Roughness = material.roughness,
		        .F0 = material.f0,
		        .AlphaCutoff = material.alphaCutoff,
		        .AlphaMode = material.alphaMode,
		        .TextureFlags = material.textureFlags,
		        .SubsurfaceColor = material.subsurfaceColor,
		        .SubsurfaceStrength = material.subsurfaceStrength});
	}

	std::unordered_map<const GPUMesh*, MeshHitDataOffsets> meshOffsets;
	std::uint32_t validInstanceCount = 0u;
	std::uint32_t skippedSkinnedInstanceCount = 0u;
	std::uint32_t skippedMissingHitDataCount = 0u;
	std::uint32_t skippedInvalidMaterialCount = 0u;
	for (std::uint32_t instanceIndex = 0u; instanceIndex < static_cast<std::uint32_t>(sceneData.meshInstances.size()); ++instanceIndex)
	{
		const MeshDraw& draw = sceneData.meshInstances[instanceIndex];
		if (draw.Geometry.MeshKind == RenderMeshKind::Skeletal)
		{
			++skippedSkinnedInstanceCount;
			continue;
		}
		if (draw.Material.Slot >= materials.size())
		{
			++skippedInvalidMaterialCount;
			continue;
		}

		const GPUMesh* gpuMesh = draw.Geometry.GpuMesh;
		if (gpuMesh == nullptr || !gpuMesh->HasRTIndirectSpecularHitData())
		{
			++skippedMissingHitDataCount;
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
			    .VertexCount = static_cast<std::uint32_t>(gpuMesh->GetRTIndirectSpecularHitVertices().size()),
			    .IndexCount = static_cast<std::uint32_t>(gpuMesh->GetRTIndirectSpecularHitIndices().size())};
			vertices.insert(
			    vertices.end(),
			    gpuMesh->GetRTIndirectSpecularHitVertices().begin(),
			    gpuMesh->GetRTIndirectSpecularHitVertices().end());
			indices.insert(
			    indices.end(),
			    gpuMesh->GetRTIndirectSpecularHitIndices().begin(),
			    gpuMesh->GetRTIndirectSpecularHitIndices().end());
			meshOffsets.emplace(gpuMesh, offsets);
		}

		instances[instanceIndex] = RTIndirectSpecularHitInstance{
		    .FirstVertex = offsets.FirstVertex,
		    .FirstIndex = offsets.FirstIndex,
		    .VertexCount = offsets.VertexCount,
		    .IndexCount = offsets.IndexCount,
		    .MaterialSlot = draw.Material.Slot,
		    .Flags = RTIndirectSpecularHitInstanceFlag_Valid};
		++validInstanceCount;
	}

	if (validInstanceCount == 0u || vertices.empty() || indices.empty() || materials.empty())
	{
		static bool loggedNoHitData = false;
		if (!loggedNoHitData)
		{
			loggedNoHitData = true;
			SPDLOG_LOGGER_WARN(
			    g_rtIndirectSpecularHitDataLogger,
			    "RTIndirectSpecular hit-data unavailable: validInstances=0 totalInstances={} skippedSkinned={} skippedMissingHitData={} skippedInvalidMaterial={}.",
			    sceneData.meshInstances.size(),
			    skippedSkinnedInstanceCount,
			    skippedMissingHitDataCount,
			    skippedInvalidMaterialCount);
		}
		return {};
	}

	RTIndirectSpecularHitDataFrameData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_instanceCount = static_cast<std::uint32_t>(instances.size());
	frameData.m_materialCount = static_cast<std::uint32_t>(materials.size());
	if (!UploadStructuredBuffer(
	        renderHardwareInterface,
	        vertices,
	        L"RTIndirectSpecularHitVertices",
	        frameData.m_vertexBuffer,
	        frameData.m_vertexView,
	        frameData.m_vertexShaderResourceView) ||
	    !UploadStructuredBuffer(
	        renderHardwareInterface,
	        indices,
	        L"RTIndirectSpecularHitIndices",
	        frameData.m_indexBuffer,
	        frameData.m_indexView,
	        frameData.m_indexShaderResourceView) ||
	    !UploadStructuredBuffer(
	        renderHardwareInterface,
	        instances,
	        L"RTIndirectSpecularHitInstances",
	        frameData.m_instanceBuffer,
	        frameData.m_instanceView,
	        frameData.m_instanceShaderResourceView) ||
	    !UploadStructuredBuffer(
	        renderHardwareInterface,
	        materials,
	        L"RTIndirectSpecularHitMaterials",
	        frameData.m_materialBuffer,
	        frameData.m_materialView,
	        frameData.m_materialShaderResourceView))
	{
		SPDLOG_LOGGER_WARN(
		    g_rtIndirectSpecularHitDataLogger,
		    "RTIndirectSpecular hit-data upload failed: vertices={} indices={} instances={} materials={}.",
		    vertices.size(),
		    indices.size(),
		    instances.size(),
		    materials.size());
		frameData.Release();
		return {};
	}

	static bool loggedFirstHitData = false;
	if (!loggedFirstHitData)
	{
		loggedFirstHitData = true;
		SPDLOG_LOGGER_INFO(
		    g_rtIndirectSpecularHitDataLogger,
		    "RTIndirectSpecular hit-data ready: validInstances={} totalInstances={} uniqueMeshes={} vertices={} indices={} materials={} skippedSkinned={} skippedMissingHitData={} skippedInvalidMaterial={}.",
		    validInstanceCount,
		    sceneData.meshInstances.size(),
		    meshOffsets.size(),
		    vertices.size(),
		    indices.size(),
		    materials.size(),
		    skippedSkinnedInstanceCount,
		    skippedMissingHitDataCount,
		    skippedInvalidMaterialCount);
	}

	return frameData;
}

void RTIndirectSpecularHitDataFrameData::Release() noexcept
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
