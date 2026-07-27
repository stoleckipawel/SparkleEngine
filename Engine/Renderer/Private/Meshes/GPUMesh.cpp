#include "PCH.h"
#include "Meshes/GPUMesh.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GPUMeshPreparation.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshData.h"

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
    GPUMeshPreparedData preparedData)
{
	if (!preparedData.IsValid())
	{
		return false;
	}

	const MeshData& meshData =
	    preparedData.Source.GetResource()->GetMeshData();
	m_renderHardwareInterface = &renderHardwareInterface;
	if (!m_renderHardwareInterface->GetResourceService().CreateVertexBuffer(
	        meshData.GetVertexData(),
	        meshData.GetVertexBufferSize(),
	        static_cast<std::uint32_t>(sizeof(VertexData)),
	        L"GPUMesh_VertexBuffer",
	        m_vertexBuffer,
	        m_vertexBufferView))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshLogger, "[GPUMesh] Failed to create vertex buffer");
		return false;
	}
	if (!m_renderHardwareInterface->GetResourceService().CreateIndexBuffer(
	        meshData.GetIndexData(),
	        meshData.GetIndexBufferSize(),
	        RhiIndexFormat::UInt32,
	        L"GPUMesh_IndexBuffer",
	        m_indexBuffer,
	        m_indexBufferView))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshLogger, "[GPUMesh] Failed to create index buffer");
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_vertexBuffer);
		m_vertexBuffer = {};
		return false;
	}

	m_vertexCount = meshData.GetVertexCount();
	m_indexCount = meshData.GetIndexCount();
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

	if (!m_skinInfluences.Upload(
	        renderHardwareInterface,
	        preparedData.GpuSkinInfluences))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshLogger, "[GPUMesh] Failed to create skin influence resources");
		return false;
	}
	if (!m_morphTargets.Upload(
	        renderHardwareInterface,
	        std::move(preparedData.MorphTargetDeltas),
	        preparedData.MorphTargetCount))
	{
		return false;
	}

	return true;
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
