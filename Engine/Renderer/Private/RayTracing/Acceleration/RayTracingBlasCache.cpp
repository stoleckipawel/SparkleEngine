#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Acceleration/RayTracingBlasGeometryBuilder.h"

#include "Meshes/GpuMeshCache.h"
#include "Commands/RenderCommandContext.h"
#include "Meshes/GpuMesh.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <utility>

static const auto g_rayTracingBlasCacheLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

bool RayTracingBlasCache::BlasHandle::IsValid() const noexcept
{
	return resource && gpuAddress != 0;
}

bool RayTracingBlasCache::SkinnedEntryKey::operator==(const SkinnedEntryKey& other) const noexcept
{
	return Mesh == other.Mesh && GpuSceneSlot == other.GpuSceneSlot;
}

std::size_t RayTracingBlasCache::SkinnedEntryKeyHash::operator()(const SkinnedEntryKey& key) const noexcept
{
	const std::size_t meshHash = std::hash<std::uint64_t>{}(key.Mesh.Value);
	const std::size_t slotHash = std::hash<std::uint32_t>{}(key.GpuSceneSlot);
	return meshHash ^ (slotHash + 0x9e3779b9u + (meshHash << 6u) + (meshHash >> 2u));
}

RayTracingBlasCache::RayTracingBlasCache(RenderHardwareInterface& renderHardwareInterface, const GpuMeshCache& meshes) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface),
    m_meshes(&meshes)
{
}

RayTracingBlasCache::~RayTracingBlasCache() noexcept
{
	Clear();
}

void RayTracingBlasCache::BeginFrame() noexcept
{
	m_currentFrameStats = {};
	for (auto& [mesh, entry] : m_entries)
	{
		entry.touchedThisFrame = false;
	}
	for (auto& [key, entry] : m_skinnedEntries)
	{
		entry.touchedThisFrame = false;
	}
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureBlas(
    RenderCommandContext& commandContext,
    const GpuMesh& gpuMesh,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	if (m_renderHardwareInterface == nullptr || !gpuMesh.IsValid())
	{
		Diagnostics::Fatal(
		    g_rayTracingBlasCacheLogger,
		    __FILE__,
		    __LINE__,
		    "BLAS cache received no render hardware interface or an invalid GPU mesh.");
	}

	++m_currentFrameStats.referencedMeshCount;

	const RhiRayTracingGeometryDesc geometry = gpuMesh.GetRayTracingGeometry();
	auto [it, inserted] = m_entries.try_emplace(gpuMesh.GetHandle());
	Entry& entry = it->second;
	entry.touchedThisFrame = true;

	const bool geometryChanged = inserted || !GeometryMatches(entry, geometry);
	if (!geometryChanged && entry.accelerationStructureBuffer)
	{
		++m_currentFrameStats.reusedBlasCount;
		return BuildHandle(entry);
	}

	return BuildBlas(commandContext, geometry, entry, diagnostics);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureBlas(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const MeshDraw& draw,
    std::uint32_t gpuSceneSlot,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	if (RayTracingBlasGeometryBuilder::IsSkinnedDraw(draw))
	{
		return EnsureSkinnedBlas(commandContext, preparedScene, draw, gpuSceneSlot, diagnostics);
	}

	const GpuMesh* gpuMesh = m_meshes != nullptr ? m_meshes->Resolve(draw.Geometry.Mesh) : nullptr;
	if (gpuMesh == nullptr)
	{
		Diagnostics::Fatal(
		    g_rayTracingBlasCacheLogger,
		    __FILE__,
		    __LINE__,
		    "BLAS cache could not resolve the mesh referenced by the render scene.");
	}
	return EnsureBlas(commandContext, *gpuMesh, diagnostics);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::BuildBlas(
    RenderCommandContext& commandContext,
    const RhiRayTracingGeometryDesc& geometry,
    Entry& entry,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetRayTracingService().GetBottomLevelAccelerationStructurePrebuildInfo(geometry);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
	{
		Diagnostics::Fatal(g_rayTracingBlasCacheLogger, __FILE__, __LINE__, "BLAS prebuild sizing produced an unusable resource layout.");
	}

	EnsureEntryResources(prebuildInfo, entry);

	entry.geometry = geometry;
	TrackBuildResources(commandContext, entry);

	{
		auto blasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuScope("BLAS Build") : ScopedGpuScope{};
		commandContext.BuildBottomLevelAccelerationStructure(
		    geometry,
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(entry.scratchBuffer),
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(entry.accelerationStructureBuffer));
	}
	++m_currentFrameStats.builtBlasCount;
	BlasHandle handle = BuildHandle(entry);
	handle.builtThisFrame = true;
	return handle;
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureSkinnedBlas(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const MeshDraw& draw,
    std::uint32_t gpuSceneSlot,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	const GpuMesh* gpuMesh = m_meshes != nullptr ? m_meshes->Resolve(draw.Geometry.Mesh) : nullptr;
	if (m_renderHardwareInterface == nullptr || gpuMesh == nullptr || !gpuMesh->IsValid())
	{
		Diagnostics::Fatal(g_rayTracingBlasCacheLogger, __FILE__, __LINE__, "Skinned BLAS cache could not resolve a valid GPU mesh.");
	}

	++m_currentFrameStats.referencedMeshCount;

	SkinnedEntryKey key{.Mesh = draw.Geometry.Mesh, .GpuSceneSlot = gpuSceneSlot};
	auto [it, inserted] = m_skinnedEntries.try_emplace(key);
	(void) inserted;
	Entry& entry = it->second;
	entry.touchedThisFrame = true;

	return BuildBlas(commandContext, BuildSkinnedGeometry(preparedScene, draw, entry), entry, diagnostics);
}

RayTracingBlasCache::BuildStats RayTracingBlasCache::EndFrame() noexcept
{
	for (auto it = m_entries.begin(); it != m_entries.end();)
	{
		if (it->second.touchedThisFrame)
		{
			++it;
			continue;
		}

		ReleaseEntryResources(it->second);
		it = m_entries.erase(it);
	}
	for (auto it = m_skinnedEntries.begin(); it != m_skinnedEntries.end();)
	{
		if (it->second.touchedThisFrame)
		{
			++it;
			continue;
		}

		ReleaseEntryResources(it->second);
		it = m_skinnedEntries.erase(it);
	}

	return m_currentFrameStats;
}

void RayTracingBlasCache::Clear() noexcept
{
	for (auto& [mesh, entry] : m_entries)
	{
		ReleaseEntryResources(entry);
	}
	for (auto& [key, entry] : m_skinnedEntries)
	{
		ReleaseEntryResources(entry);
	}

	m_entries.clear();
	m_skinnedEntries.clear();
	m_currentFrameStats = {};
}

void RayTracingBlasCache::ReleaseEntryResources(Entry& entry) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		entry = {};
		return;
	}

	if (entry.scratchBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.scratchBuffer);
	}
	if (entry.dynamicVertexBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.dynamicVertexBuffer);
	}
	if (entry.accelerationStructureBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.accelerationStructureBuffer);
	}

	entry = {};
}

RhiRayTracingGeometryDesc RayTracingBlasCache::BuildSkinnedGeometry(
    const PreparedRenderScene& preparedScene,
    const MeshDraw& draw,
    Entry& entry) noexcept
{
	const GpuMesh* resolvedMesh = m_meshes != nullptr ? m_meshes->Resolve(draw.Geometry.Mesh) : nullptr;
	if (m_renderHardwareInterface == nullptr || resolvedMesh == nullptr
	    || draw.Skinning.JointMatrixOffset == kInvalidMeshInstanceJointMatrixOffset)
	{
		Diagnostics::Fatal(
		    g_rayTracingBlasCacheLogger,
		    __FILE__,
		    __LINE__,
		    "Skinned BLAS input has no GPU mesh, render hardware interface, or joint-matrix range.");
	}

	const GpuMesh& gpuMesh = *resolvedMesh;
	RayTracingBlasGeometryBuilder::ComputeSkinnedPositions(preparedScene, draw, gpuMesh, m_skinnedPositionScratch);
	ReplaceDynamicVertexBuffer(m_skinnedPositionScratch, entry);
	return BuildSkinnedGeometryDesc(gpuMesh, entry, static_cast<std::uint32_t>(m_skinnedPositionScratch.size()));
}

void RayTracingBlasCache::ReplaceDynamicVertexBuffer(std::span<const DirectX::XMFLOAT3> positions, Entry& entry) noexcept
{
	if (entry.dynamicVertexBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.dynamicVertexBuffer);
		entry.dynamicVertexBuffer = {};
		entry.dynamicVertexBufferView = {};
	}

	if (!m_renderHardwareInterface->GetResourceService().CreateVertexBuffer(
	        positions.data(),
	        positions.size() * sizeof(DirectX::XMFLOAT3),
	        static_cast<std::uint32_t>(sizeof(DirectX::XMFLOAT3)),
	        L"RayTracingSkinnedBlasVertices",
	        entry.dynamicVertexBuffer,
	        entry.dynamicVertexBufferView)
	    || !entry.dynamicVertexBuffer)
	{
		Diagnostics::Fatal(g_rayTracingBlasCacheLogger, __FILE__, __LINE__, "Skinned BLAS dynamic vertex-buffer allocation failed.");
	}
}

RhiRayTracingGeometryDesc RayTracingBlasCache::BuildSkinnedGeometryDesc(
    const GpuMesh& gpuMesh,
    const Entry& entry,
    std::uint32_t vertexCount) const noexcept
{
	const RhiIndexBufferView indexBufferView = gpuMesh.GetIndexBufferView();
	RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

	return RhiRayTracingGeometryDesc{
	    .VertexBuffer = RhiRayTracingBufferBinding{.Resource = resources.GetResourceHandle(entry.dynamicVertexBuffer)},
	    .VertexStrideInBytes = entry.dynamicVertexBufferView.StrideInBytes,
	    .VertexCount = vertexCount,
	    .IndexBuffer = RhiRayTracingBufferBinding{.Resource = resources.GetResourceHandle(gpuMesh.GetIndexBufferResource())},
	    .IndexCount = gpuMesh.GetIndexCount(),
	    .IndexFormat = indexBufferView.Format,
	    .Opaque = true};
}

void RayTracingBlasCache::EnsureEntryResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo, Entry& entry) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		Diagnostics::Fatal(g_rayTracingBlasCacheLogger, __FILE__, __LINE__, "BLAS resource allocation has no render hardware interface.");
	}

	if (entry.scratchBuffer && entry.scratchBufferSizeInBytes < prebuildInfo.ScratchDataSizeInBytes)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.scratchBuffer);
		entry.scratchBuffer = {};
		entry.scratchBufferSizeInBytes = 0;
	}
	if (entry.accelerationStructureBuffer && entry.accelerationStructureSizeInBytes < prebuildInfo.ResultDataMaxSizeInBytes)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.accelerationStructureBuffer);
		entry.accelerationStructureBuffer = {};
		entry.accelerationStructureSizeInBytes = 0;
	}

	if (!entry.scratchBuffer)
	{
		const std::uint64_t alignedScratchSize = RayTracingBlasGeometryBuilder::AlignRayTracingBufferSize(
		    prebuildInfo.ScratchDataSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.ScratchBufferByteAlignment);
		entry.scratchBuffer =
		    m_renderHardwareInterface->GetRayTracingService().CreateRayTracingScratchBuffer(alignedScratchSize, L"RayTracingBlasScratch");
		entry.scratchBufferSizeInBytes = alignedScratchSize;
	}
	if (!entry.accelerationStructureBuffer)
	{
		const std::uint64_t alignedAccelerationStructureSize = RayTracingBlasGeometryBuilder::AlignRayTracingBufferSize(
		    prebuildInfo.ResultDataMaxSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.AccelerationStructureByteAlignment);
		entry.accelerationStructureBuffer = m_renderHardwareInterface->GetRayTracingService().CreateRayTracingAccelerationStructureBuffer(
		    alignedAccelerationStructureSize,
		    ERhiRayTracingAccelerationStructureType::BottomLevel,
		    L"RayTracingBlas");
		entry.accelerationStructureSizeInBytes = alignedAccelerationStructureSize;
	}

	if (!entry.scratchBuffer || !entry.accelerationStructureBuffer)
	{
		Diagnostics::Fatal(g_rayTracingBlasCacheLogger, __FILE__, __LINE__, "BLAS scratch or acceleration-structure allocation failed.");
	}
}

void RayTracingBlasCache::TrackBuildResources(RenderCommandContext& commandContext, const Entry& entry) const noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

	commandContext.TrackResource(resources.GetResourceHandle(entry.scratchBuffer));
	commandContext.TrackResource(resources.GetResourceHandle(entry.accelerationStructureBuffer));
}

bool RayTracingBlasCache::GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return RayTracingBlasGeometryBuilder::GeometryEquals(entry.geometry, geometry);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::BuildHandle(const Entry& entry) const noexcept
{
	if (m_renderHardwareInterface == nullptr || !entry.accelerationStructureBuffer)
	{
		Diagnostics::Fatal(g_rayTracingBlasCacheLogger, __FILE__, __LINE__, "BLAS cache entry has no acceleration-structure storage.");
	}

	const BlasHandle handle{
	    .resource = m_renderHardwareInterface->GetResourceService().GetResourceHandle(entry.accelerationStructureBuffer),
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(entry.accelerationStructureBuffer)};
	if (!handle.IsValid())
	{
		Diagnostics::Fatal(g_rayTracingBlasCacheLogger, __FILE__, __LINE__, "BLAS cache entry has no GPU resource or address.");
	}
	return handle;
}
