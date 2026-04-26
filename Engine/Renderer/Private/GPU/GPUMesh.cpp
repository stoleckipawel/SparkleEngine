#include "PCH.h"
#include "GPU/GPUMesh.h"

#include "GPU/CommandContext.h"
#include "Scene/Meshes/MeshData.h"

static const auto g_gpuMeshLogger = Logging::GetOrCreateLogger("Renderer.GPUMesh");

GPUMesh::~GPUMesh() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_vertexBuffer)
		{
			m_renderHardwareInterface->ReleaseOwnedResource(m_vertexBuffer);
			m_vertexBuffer = {};
		}

		if (m_indexBuffer)
		{
			m_renderHardwareInterface->ReleaseOwnedResource(m_indexBuffer);
			m_indexBuffer = {};
		}
	}
}

bool GPUMesh::Upload(RenderHardwareInterface& renderHardwareInterface, const MeshData& meshData)
{
	if (!meshData.IsValid())
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshLogger, "[GPUMesh] Cannot upload invalid MeshData (empty vertices or indices)");
		return false;
	}

	m_renderHardwareInterface = &renderHardwareInterface;
	if (!m_renderHardwareInterface->CreateVertexBuffer(
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
	if (!m_renderHardwareInterface->CreateIndexBuffer(
	        meshData.GetIndexData(),
	        meshData.GetIndexBufferSize(),
	        RhiIndexFormat::UInt32,
	        L"GPUMesh_IndexBuffer",
	        m_indexBuffer,
	        m_indexBufferView))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshLogger, "[GPUMesh] Failed to create index buffer");
		m_renderHardwareInterface->ReleaseOwnedResource(m_vertexBuffer);
		m_vertexBuffer = {};
		return false;
	}

	m_vertexCount = meshData.GetVertexCount();
	m_indexCount = meshData.GetIndexCount();

	SPDLOG_LOGGER_TRACE(g_gpuMeshLogger, "[GPUMesh] Uploaded mesh buffers");

	return true;
}

void GPUMesh::Bind(CommandContext& cmd) const noexcept
{
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
