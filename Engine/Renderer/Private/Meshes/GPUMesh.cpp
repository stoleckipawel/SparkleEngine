#include "PCH.h"
#include "Meshes/GPUMesh.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GPUMeshPreparation.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RhiUploadService.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshData.h"

#include <span>
#include <utility>

static const auto g_gpuMeshLogger = Logging::GetOrCreateLogger("Renderer.GPUMesh");

GPUMesh::GPUMesh(GpuMeshHandle handle) noexcept :
	m_handle(handle)
{
}

GPUMesh::~GPUMesh() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_vertexBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_vertexBuffer);
			m_vertexBuffer = {};
		}

		if (m_indexBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_indexBuffer);
			m_indexBuffer = {};
		}
	}
}

bool GPUMesh::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    RenderCommandList& commandList,
    GPUMeshPreparedData preparedData)
{
	if (!preparedData.IsValid())
	{
		return false;
	}

	m_renderHardwareInterface = &renderHardwareInterface;
	const MeshData& meshData =
	    preparedData.Source.GetResource()->GetMeshData();
	if (!CreateGeometryBuffers(
	        commandList,
	        meshData))
	{
		return false;
	}

	if (!CreateDeformationBuffers(
	        commandList,
	        preparedData))
	{
		return false;
	}

	CommitPreparedData(
	    std::move(preparedData));
	return true;
}

bool GPUMesh::CreateGeometryBuffers(
    RenderCommandList& commandList,
    const MeshData& meshData)
{
	if (!CreateVertexBuffer(
	        commandList,
	        meshData))
	{
		SPDLOG_LOGGER_ERROR(
		    g_gpuMeshLogger,
		    "[GPUMesh] Failed to create vertex buffer");
		return false;
	}

	if (!CreateIndexBuffer(
	        commandList,
	        meshData))
	{
		SPDLOG_LOGGER_ERROR(
		    g_gpuMeshLogger,
		    "[GPUMesh] Failed to create index buffer");
		return false;
	}

	m_vertexCount = meshData.GetVertexCount();
	m_indexCount = meshData.GetIndexCount();
	return true;
}

bool GPUMesh::CreateVertexBuffer(
    RenderCommandList& commandList,
    const MeshData& meshData)
{
	RhiResourceService& resources =
	    m_renderHardwareInterface->GetResourceService();
	m_vertexBuffer = resources.CreateBufferResource(
	    RhiBufferResourceDesc{
	        .SizeInBytes =
	            meshData.GetVertexBufferSize(),
	        .StrideInBytes =
	            sizeof(VertexData),
	        .Kind = RhiBufferKind::Vertex,
	        .AllowRayTracingBuildInput = true},
	    ResourceState::CopyDest,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"GPUMesh_VertexBuffer");
	if (!m_vertexBuffer)
	{
		return false;
	}

	const std::span<const VertexData> vertices{
	    meshData.vertices};
	if (!m_renderHardwareInterface->GetUploadService()
	         .UploadBuffer(
	             commandList,
	             m_vertexBuffer,
	             std::as_bytes(vertices),
	             ResourceState::Common,
	             L"GPUMesh_VertexUpload"))
	{
		resources.ReleaseOwnedResource(
		    m_vertexBuffer);
		m_vertexBuffer = {};
		return false;
	}

	m_vertexBufferView = RhiVertexBufferView{
	    .BufferLocation =
	        resources.GetResourceGpuVirtualAddress(
	            m_vertexBuffer),
	    .SizeInBytes = static_cast<std::uint32_t>(
	        meshData.GetVertexBufferSize()),
	    .StrideInBytes =
	        sizeof(VertexData)};
	return m_vertexBufferView.BufferLocation != 0;
}

bool GPUMesh::CreateIndexBuffer(
    RenderCommandList& commandList,
    const MeshData& meshData)
{
	RhiResourceService& resources =
	    m_renderHardwareInterface->GetResourceService();
	m_indexBuffer = resources.CreateBufferResource(
	    RhiBufferResourceDesc{
	        .SizeInBytes =
	            meshData.GetIndexBufferSize(),
	        .Kind = RhiBufferKind::Index,
	        .AllowRayTracingBuildInput = true},
	    ResourceState::CopyDest,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"GPUMesh_IndexBuffer");
	if (!m_indexBuffer)
	{
		return false;
	}

	const std::span<const std::uint32_t> indices{
	    meshData.indices};
	if (!m_renderHardwareInterface->GetUploadService()
	         .UploadBuffer(
	             commandList,
	             m_indexBuffer,
	             std::as_bytes(indices),
	             ResourceState::Common,
	             L"GPUMesh_IndexUpload"))
	{
		resources.ReleaseOwnedResource(
		    m_indexBuffer);
		m_indexBuffer = {};
		return false;
	}

	m_indexBufferView = RhiIndexBufferView{
	    .BufferLocation =
	        resources.GetResourceGpuVirtualAddress(
	            m_indexBuffer),
	    .SizeInBytes = static_cast<std::uint32_t>(
	        meshData.GetIndexBufferSize()),
	    .Format = RhiIndexFormat::UInt32};
	return m_indexBufferView.BufferLocation != 0;
}

bool GPUMesh::CreateDeformationBuffers(
    RenderCommandList& commandList,
    GPUMeshPreparedData& preparedData)
{
	if (!m_skinInfluences.Upload(
	        *m_renderHardwareInterface,
	        commandList,
	        preparedData.GpuSkinInfluences))
	{
		SPDLOG_LOGGER_ERROR(
		    g_gpuMeshLogger,
		    "[GPUMesh] Failed to create skin influence resources");
		return false;
	}

	return m_morphTargets.Upload(
	    *m_renderHardwareInterface,
	    commandList,
	    std::move(
	        preparedData.MorphTargetDeltas),
	    preparedData.MorphTargetCount);
}

void GPUMesh::CommitPreparedData(
    GPUMeshPreparedData&& preparedData)
{
	m_localBounds = GPUMeshBounds{
	    .Min = preparedData.LocalBoundsMin,
	    .Max = preparedData.LocalBoundsMax,
	    .Valid = preparedData.HasLocalBounds};
	m_rayTracingHitVertices =
	    std::move(preparedData.RayTracingVertices);
	m_rayTracingHitIndices =
	    std::move(preparedData.RayTracingIndices);
	m_cpuSkinInfluences =
	    std::move(preparedData.SkinInfluences);
}

void GPUMesh::Bind(RenderCommandContext& cmd) const noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

		cmd.TrackResource(resources.GetResourceHandle(m_vertexBuffer));
		cmd.TrackResource(resources.GetResourceHandle(m_indexBuffer));
	}

	cmd.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
	cmd.BindVertexBuffer(GetVertexBufferView());
	cmd.BindIndexBuffer(GetIndexBufferView());
}

RhiVertexBufferView GPUMesh::GetVertexBufferView() const noexcept
{
	return m_vertexBufferView;
}

RhiIndexBufferView GPUMesh::GetIndexBufferView() const noexcept
{
	return m_indexBufferView;
}

RhiRayTracingGeometryDesc GPUMesh::GetRayTracingGeometry() const noexcept
{
	RhiResourceService* const resources =
	    m_renderHardwareInterface != nullptr ? &m_renderHardwareInterface->GetResourceService() : nullptr;
	const RhiResourceHandle vertexResource =
	    resources != nullptr ? resources->GetResourceHandle(m_vertexBuffer) : RhiResourceHandle{};
	const RhiResourceHandle indexResource =
	    resources != nullptr ? resources->GetResourceHandle(m_indexBuffer) : RhiResourceHandle{};

	return RhiRayTracingGeometryDesc{
	    .VertexBuffer = RhiRayTracingBufferBinding{.Resource = vertexResource},
	    .VertexStrideInBytes = m_vertexBufferView.StrideInBytes,
	    .VertexCount = m_vertexCount,
	    .IndexBuffer = RhiRayTracingBufferBinding{.Resource = indexResource},
	    .IndexCount = m_indexCount,
	    .IndexFormat = m_indexBufferView.Format,
	    .Opaque = true};
}
