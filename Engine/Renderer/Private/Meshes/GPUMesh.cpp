#include "PCH.h"
#include "Meshes/GPUMesh.h"

#include "Commands/RenderCommandContext.h"
#include "Scene/Meshes/MeshData.h"

#include <algorithm>

static const auto g_gpuMeshLogger = Logging::GetOrCreateLogger("Renderer.GPUMesh");

namespace
{
	GPUMeshBounds ComputeLocalBounds(const MeshData& meshData) noexcept
	{
		GPUMeshBounds bounds{};
		for (const VertexData& vertex : meshData.vertices)
		{
			if (!bounds.Valid)
			{
				bounds.Min = vertex.position;
				bounds.Max = vertex.position;
				bounds.Valid = true;
				continue;
			}

			bounds.Min.x = (std::min)(bounds.Min.x, vertex.position.x);
			bounds.Min.y = (std::min)(bounds.Min.y, vertex.position.y);
			bounds.Min.z = (std::min)(bounds.Min.z, vertex.position.z);
			bounds.Max.x = (std::max)(bounds.Max.x, vertex.position.x);
			bounds.Max.y = (std::max)(bounds.Max.y, vertex.position.y);
			bounds.Max.z = (std::max)(bounds.Max.z, vertex.position.z);
		}
		return bounds;
	}
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

bool GPUMesh::Upload(RenderHardwareInterface& renderHardwareInterface, const MeshData& meshData)
{
	return Upload(renderHardwareInterface, GPUMeshUploadDesc{.meshData = meshData});
}

bool GPUMesh::Upload(RenderHardwareInterface& renderHardwareInterface, const GPUMeshUploadDesc& uploadDesc)
{
	const MeshData& meshData = uploadDesc.meshData;
	if (!meshData.IsValid())
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshLogger, "[GPUMesh] Cannot upload invalid MeshData (empty vertices or indices)");
		return false;
	}

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
	m_localBounds = ComputeLocalBounds(meshData);
	m_rayTracingHitVertices.clear();
	m_rayTracingHitVertices.reserve(meshData.vertices.size());
	for (const VertexData& vertex : meshData.vertices)
	{
		m_rayTracingHitVertices.push_back(
		    RayTracingHitVertex{
		        .Position = vertex.position,
		        .Normal = vertex.normal,
		        .Tangent = vertex.tangent,
		        .TexCoord0 = vertex.uv});
	}
	m_rayTracingHitIndices.assign(meshData.indices.begin(), meshData.indices.end());

	if (!m_skinInfluences.Upload(renderHardwareInterface, m_vertexCount, uploadDesc.skinInfluences))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshLogger, "[GPUMesh] Failed to create skin influence resources");
		return false;
	}

	SPDLOG_LOGGER_TRACE(g_gpuMeshLogger, "[GPUMesh] Uploaded mesh buffers");

	return true;
}

void GPUMesh::Bind(RenderCommandContext& cmd) const noexcept
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

RhiRayTracingGeometryDesc GPUMesh::GetRayTracingGeometry() const noexcept
{
	return RhiRayTracingGeometryDesc{
	    .VertexBuffer = m_vertexBufferView.BufferLocation,
	    .VertexStrideInBytes = m_vertexBufferView.StrideInBytes,
	    .VertexCount = m_vertexCount,
	    .IndexBuffer = m_indexBufferView.BufferLocation,
	    .IndexCount = m_indexCount,
	    .IndexFormat = m_indexBufferView.Format,
	    .Opaque = true};
}
