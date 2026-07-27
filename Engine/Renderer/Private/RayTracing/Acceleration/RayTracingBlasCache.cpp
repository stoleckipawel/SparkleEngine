#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Acceleration/RayTracingBlasGeometryBuilder.h"

#include "Meshes/GPUMeshCache.h"
#include "Commands/RenderCommandContext.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <utility>

static const auto g_rayTracingBlasCacheLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

bool RayTracingBlasCache::BlasHandle::IsValid() const noexcept
{
	return resource && gpuAddress != 0;
}

bool RayTracingBlasCache::SkinnedEntryKey::operator==(
    const SkinnedEntryKey& other) const noexcept
{
	return Mesh == other.Mesh &&
	       GpuSceneSlot == other.GpuSceneSlot;
}

std::size_t
RayTracingBlasCache::SkinnedEntryKeyHash::operator()(
    const SkinnedEntryKey& key) const noexcept
{
	const std::size_t meshHash =
	    std::hash<std::uint64_t>{}(key.Mesh.Value);
	const std::size_t slotHash =
	    std::hash<std::uint32_t>{}(key.GpuSceneSlot);
	return meshHash ^
	       (slotHash + 0x9e3779b9u + (meshHash << 6u) +
	        (meshHash >> 2u));
}

RayTracingBlasCache::RayTracingBlasCache(
    RenderHardwareInterface& renderHardwareInterface,
    const GPUMeshCache& meshes) noexcept :
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
    RenderCommandContext& cmd,
    const GPUMesh& gpuMesh,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{

	if (m_renderHardwareInterface == nullptr || !gpuMesh.IsValid())
	{
		return {};
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

	return BuildBlas(cmd, geometry, entry, diagnostics);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureBlas(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    std::uint32_t gpuSceneSlot,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	if (RayTracingBlasGeometryBuilder::IsSkinnedDraw(draw))
	{
		return EnsureSkinnedBlas(
		    cmd,
		    sceneData,
		    draw,
		    gpuSceneSlot,
		    diagnostics);
	}

	const GPUMesh* gpuMesh =
	    m_meshes != nullptr ? m_meshes->Resolve(draw.Geometry.Mesh) : nullptr;
	if (gpuMesh == nullptr)
	{
		return {};
	}
	return EnsureBlas(cmd, *gpuMesh, diagnostics);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::BuildBlas(
    RenderCommandContext& cmd,
    const RhiRayTracingGeometryDesc& geometry,
    Entry& entry,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetRayTracingService().GetBottomLevelAccelerationStructurePrebuildInfo(geometry);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_rayTracingBlasCacheLogger,
		    "RayTracingBlasCache: skipping BLAS build because prebuild info was invalid (vertexCount={}, indexCount={}).",
		    geometry.VertexCount,
		    geometry.IndexCount);
		return {};
	}

	if (!EnsureEntryResources(geometry, prebuildInfo, entry))
	{
		return {};
	}

	entry.geometry = geometry;
	TrackBuildResources(cmd, entry);

	{
		auto blasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuScope("BLAS Build") : ScopedGpuScope{};
		cmd.BuildBottomLevelAccelerationStructure(
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
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    std::uint32_t gpuSceneSlot,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	const GPUMesh* gpuMesh =
	    m_meshes != nullptr ? m_meshes->Resolve(draw.Geometry.Mesh) : nullptr;
	if (m_renderHardwareInterface == nullptr || gpuMesh == nullptr || !gpuMesh->IsValid())
	{
		return {};
	}

	++m_currentFrameStats.referencedMeshCount;

	SkinnedEntryKey key{
	    .Mesh = draw.Geometry.Mesh,
	    .GpuSceneSlot = gpuSceneSlot};
	auto [it, inserted] = m_skinnedEntries.try_emplace(key);
	(void)inserted;
	Entry& entry = it->second;
	entry.touchedThisFrame = true;

	RhiRayTracingGeometryDesc geometry{};
	if (!BuildSkinnedGeometry(sceneData, draw, entry, geometry))
	{
		ReleaseEntryResources(entry);
		return {};
	}

	return BuildBlas(cmd, geometry, entry, diagnostics);
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

bool RayTracingBlasCache::BuildSkinnedGeometry(
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    Entry& entry,
    RhiRayTracingGeometryDesc& outGeometry) noexcept
{
	const GPUMesh* resolvedMesh =
	    m_meshes != nullptr ? m_meshes->Resolve(draw.Geometry.Mesh) : nullptr;
	if (m_renderHardwareInterface == nullptr || resolvedMesh == nullptr ||
	    draw.Skinning.JointMatrixOffset == kInvalidMeshInstanceJointMatrixOffset)
	{
		return false;
	}

	const GPUMesh& gpuMesh = *resolvedMesh;
	if (!RayTracingBlasGeometryBuilder::BuildSkinnedPositions(
	        sceneData,
	        draw,
	        gpuMesh,
	        m_skinnedPositionScratch))
	{
		return false;
	}

	if (!ReplaceDynamicVertexBuffer(m_skinnedPositionScratch, entry))
	{
		return false;
	}

	outGeometry = BuildSkinnedGeometryDesc(
	    gpuMesh,
	    entry,
	    static_cast<std::uint32_t>(m_skinnedPositionScratch.size()));
	return true;
}

bool RayTracingBlasCache::ReplaceDynamicVertexBuffer(
    std::span<const DirectX::XMFLOAT3> positions,
    Entry& entry) noexcept
{
	if (entry.dynamicVertexBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.dynamicVertexBuffer);
		entry.dynamicVertexBuffer = {};
		entry.dynamicVertexBufferView = {};
	}

	return m_renderHardwareInterface->GetResourceService().CreateVertexBuffer(
	        positions.data(),
	        positions.size() * sizeof(DirectX::XMFLOAT3),
	        static_cast<std::uint32_t>(sizeof(DirectX::XMFLOAT3)),
	        L"RayTracingSkinnedBlasVertices",
	        entry.dynamicVertexBuffer,
	        entry.dynamicVertexBufferView) &&
	       entry.dynamicVertexBuffer;
}

RhiRayTracingGeometryDesc RayTracingBlasCache::BuildSkinnedGeometryDesc(
    const GPUMesh& gpuMesh,
    const Entry& entry,
    std::uint32_t vertexCount) const noexcept
{
	const RhiIndexBufferView indexBufferView = gpuMesh.GetIndexBufferView();
	RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

	return RhiRayTracingGeometryDesc{
	    .VertexBuffer = RhiRayTracingBufferBinding{
	        .Resource = resources.GetResourceHandle(entry.dynamicVertexBuffer)},
	    .VertexStrideInBytes = entry.dynamicVertexBufferView.StrideInBytes,
	    .VertexCount = vertexCount,
	    .IndexBuffer = RhiRayTracingBufferBinding{
	        .Resource = resources.GetResourceHandle(gpuMesh.GetIndexBufferResource())},
	    .IndexCount = gpuMesh.GetIndexCount(),
	    .IndexFormat = indexBufferView.Format,
	    .Opaque = true};
}

bool RayTracingBlasCache::EnsureEntryResources(
    const RhiRayTracingGeometryDesc& geometry,
    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
    Entry& entry) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return false;
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
		SPDLOG_LOGGER_WARN(
		    g_rayTracingBlasCacheLogger,
		    "RayTracingBlasCache: failed to allocate BLAS resources for geometry (vertexCount={}, indexCount={}).",
		    geometry.VertexCount,
		    geometry.IndexCount);
		return false;
	}

	return true;
}

void RayTracingBlasCache::TrackBuildResources(
    RenderCommandContext& cmd,
    const Entry& entry) const noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

	cmd.TrackResource(resources.GetResourceHandle(entry.scratchBuffer));
	cmd.TrackResource(resources.GetResourceHandle(entry.accelerationStructureBuffer));
}

bool RayTracingBlasCache::GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return RayTracingBlasGeometryBuilder::GeometryEquals(entry.geometry, geometry);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::BuildHandle(const Entry& entry) const noexcept
{
	if (m_renderHardwareInterface == nullptr || !entry.accelerationStructureBuffer)
	{
		return {};
	}

	return BlasHandle{
	    .resource = m_renderHardwareInterface->GetResourceService().GetResourceHandle(entry.accelerationStructureBuffer),
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(entry.accelerationStructureBuffer)};
}

