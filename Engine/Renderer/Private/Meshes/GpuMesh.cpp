#include "PCH.h"
#include "Meshes/GpuMesh.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Meshes/GpuMeshPreparation.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RhiUploadService.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshData.h"

#include <cstddef>
#include <span>
#include <utility>

static const auto g_gpuMeshLogger = Logging::GetOrCreateLogger("Renderer.GpuMesh");

GpuMesh::GpuMesh(GpuMeshHandle handle) noexcept :
	m_handle(handle)
{
}

GpuMesh::~GpuMesh() noexcept
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

void GpuMesh::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    RenderCommandList& commandList,
    GpuMeshPreparedData preparedData)
{
	m_renderHardwareInterface = &renderHardwareInterface;
	const MeshData& meshData =
	    preparedData.Source.GetResource()->GetMeshData();
	CreateGeometryBuffers(commandList, meshData);
	CreateDeformationBuffers(commandList, preparedData);

	CommitPreparedData(
	    std::move(preparedData));
}

void GpuMesh::CreateGeometryBuffers(
    RenderCommandList& commandList,
    const MeshData& meshData)
{
	CreateVertexBuffer(commandList, meshData);
	CreateIndexBuffer(commandList, meshData);

	m_vertexCount = meshData.GetVertexCount();
	m_indexCount = meshData.GetIndexCount();
}

void GpuMesh::CreateVertexBuffer(
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
	    L"GpuMesh_VertexBuffer");
	if (!m_vertexBuffer)
		Diagnostics::Fatal(g_gpuMeshLogger, __FILE__, __LINE__, "GPU mesh vertex buffer creation failed.");

	const std::span<const VertexData> vertices{
	    meshData.vertices};
	if (!m_renderHardwareInterface->GetUploadService()
	         .UploadBuffer(
	             commandList,
	             m_vertexBuffer,
	             std::as_bytes(vertices),
	             ResourceState::Common,
	             L"GpuMesh_VertexUpload"))
	{
		resources.ReleaseOwnedResource(
		    m_vertexBuffer);
		m_vertexBuffer = {};
		Diagnostics::Fatal(g_gpuMeshLogger, __FILE__, __LINE__, "GPU mesh vertex upload failed.");
	}

	m_vertexBufferView = RhiVertexBufferView{
	    .BufferLocation =
	        resources.GetResourceGpuVirtualAddress(
	            m_vertexBuffer),
	    .SizeInBytes = static_cast<std::uint32_t>(
	        meshData.GetVertexBufferSize()),
	    .StrideInBytes =
	        sizeof(VertexData)};
	if (m_vertexBufferView.BufferLocation == 0)
		Diagnostics::Fatal(g_gpuMeshLogger, __FILE__, __LINE__, "GPU mesh vertex buffer has no device address.");
}

void GpuMesh::CreateIndexBuffer(
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
	    L"GpuMesh_IndexBuffer");
	if (!m_indexBuffer)
		Diagnostics::Fatal(g_gpuMeshLogger, __FILE__, __LINE__, "GPU mesh index buffer creation failed.");

	const std::span<const std::uint32_t> indices{
	    meshData.indices};
	if (!m_renderHardwareInterface->GetUploadService()
	         .UploadBuffer(
	             commandList,
	             m_indexBuffer,
	             std::as_bytes(indices),
	             ResourceState::Common,
	             L"GpuMesh_IndexUpload"))
	{
		resources.ReleaseOwnedResource(
		    m_indexBuffer);
		m_indexBuffer = {};
		Diagnostics::Fatal(g_gpuMeshLogger, __FILE__, __LINE__, "GPU mesh index upload failed.");
	}

	m_indexBufferView = RhiIndexBufferView{
	    .BufferLocation =
	        resources.GetResourceGpuVirtualAddress(
	            m_indexBuffer),
	    .SizeInBytes = static_cast<std::uint32_t>(
	        meshData.GetIndexBufferSize()),
	    .Format = RhiIndexFormat::UInt32};
	if (m_indexBufferView.BufferLocation == 0)
		Diagnostics::Fatal(g_gpuMeshLogger, __FILE__, __LINE__, "GPU mesh index buffer has no device address.");
}

void GpuMesh::CreateDeformationBuffers(
    RenderCommandList& commandList,
    GpuMeshPreparedData& preparedData)
{
	m_skinInfluences.Upload(
	    *m_renderHardwareInterface,
	    commandList,
	    preparedData.GpuSkinInfluences);
	m_morphTargets.Upload(
	    *m_renderHardwareInterface,
	    commandList,
	    std::move(
	        preparedData.MorphTargetDeltas),
	    preparedData.MorphTargetCount);
}

void GpuMesh::CommitPreparedData(
    GpuMeshPreparedData&& preparedData)
{
	m_localBounds = GpuMeshBounds{
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

void GpuMesh::Bind(RenderCommandContext& commandContext) const noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

		commandContext.TrackResource(resources.GetResourceHandle(m_vertexBuffer));
		commandContext.TrackResource(resources.GetResourceHandle(m_indexBuffer));
	}

	commandContext.SetPrimitiveTopology(GetPrimitiveTopology());
	commandContext.BindVertexBuffer(GetVertexBufferView());
	commandContext.BindIndexBuffer(GetIndexBufferView());
}

const RhiVertexInputDeclaration& GpuMesh::GetVertexInputDeclaration() const noexcept
{
	static const RhiVertexInputDeclaration declaration{
	    .Bindings = {RhiVertexInputBinding{.Binding = 0, .StrideInBytes = sizeof(VertexData)}},
	    .Elements = {
	        RhiVertexInputElement{
	            .Semantic = RhiVertexSemantic::Position,
	            .Location = 0,
	            .Binding = 0,
	            .Format = RhiVertexElementFormat::Float3,
	            .OffsetInBytes = offsetof(VertexData, position)},
	        RhiVertexInputElement{
	            .Semantic = RhiVertexSemantic::TexCoord,
	            .Location = 1,
	            .Binding = 0,
	            .Format = RhiVertexElementFormat::Float2,
	            .OffsetInBytes = offsetof(VertexData, uv)},
	        RhiVertexInputElement{
	            .Semantic = RhiVertexSemantic::Normal,
	            .Location = 2,
	            .Binding = 0,
	            .Format = RhiVertexElementFormat::Float3,
	            .OffsetInBytes = offsetof(VertexData, normal)},
	        RhiVertexInputElement{
	            .Semantic = RhiVertexSemantic::Tangent,
	            .Location = 3,
	            .Binding = 0,
	            .Format = RhiVertexElementFormat::Float4,
	            .OffsetInBytes = offsetof(VertexData, tangent)}},
	    .BindingCount = 1,
	    .ElementCount = 4};
	return declaration;
}

RhiVertexBufferView GpuMesh::GetVertexBufferView() const noexcept
{
	return m_vertexBufferView;
}

RhiIndexBufferView GpuMesh::GetIndexBufferView() const noexcept
{
	return m_indexBufferView;
}

RhiRayTracingGeometryDesc GpuMesh::GetRayTracingGeometry() const noexcept
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
