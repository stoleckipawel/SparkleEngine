#include "PCH.h"

#include "RayTracing/RayTracingBlasCache.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Math/MathUtils.h"
#include "Meshes/GPUMesh.h"

#include <utility>

static const auto g_rayTracingBlasCacheLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

namespace
{
	bool GeometryEquals(const RhiRayTracingGeometryDesc& left, const RhiRayTracingGeometryDesc& right) noexcept
	{
		return left.VertexBuffer == right.VertexBuffer && left.VertexStrideInBytes == right.VertexStrideInBytes &&
		       left.VertexCount == right.VertexCount && left.IndexBuffer == right.IndexBuffer && left.IndexCount == right.IndexCount &&
		       left.IndexFormat == right.IndexFormat && left.Opaque == right.Opaque;
	}

	std::uint64_t AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept
	{
		return alignment > 0 ? MathUtils::AlignUp(sizeInBytes, alignment) : sizeInBytes;
	}
}

RayTracingBlasCache::RayTracingBlasCache(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface)
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
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureBlas(RenderCommandContext& cmd, const GPUMesh& gpuMesh) noexcept
{
	if (m_renderHardwareInterface == nullptr || !gpuMesh.IsValid())
	{
		return {};
	}

	++m_currentFrameStats.referencedMeshCount;

	const RhiRayTracingGeometryDesc geometry = gpuMesh.GetRayTracingGeometry();
	auto [it, inserted] = m_entries.try_emplace(&gpuMesh);
	Entry& entry = it->second;
	entry.touchedThisFrame = true;

	const bool geometryChanged = inserted || !GeometryMatches(entry, geometry);
	if (!geometryChanged && entry.accelerationStructureBuffer)
	{
		++m_currentFrameStats.reusedBlasCount;
		return BuildHandle(entry);
	}

	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetBottomLevelAccelerationStructurePrebuildInfo(geometry);
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
	cmd.BuildBottomLevelAccelerationStructure(
	    geometry,
	    m_renderHardwareInterface->GetResourceGpuVirtualAddress(entry.scratchBuffer),
	    m_renderHardwareInterface->GetResourceGpuVirtualAddress(entry.accelerationStructureBuffer));
	++m_currentFrameStats.builtBlasCount;
	BlasHandle handle = BuildHandle(entry);
	handle.builtThisFrame = true;
	return handle;
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

	return m_currentFrameStats;
}

void RayTracingBlasCache::Clear() noexcept
{
	for (auto& [mesh, entry] : m_entries)
	{
		ReleaseEntryResources(entry);
	}

	m_entries.clear();
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
		m_renderHardwareInterface->ReleaseOwnedResource(entry.scratchBuffer);
	}
	if (entry.accelerationStructureBuffer)
	{
		m_renderHardwareInterface->ReleaseOwnedResource(entry.accelerationStructureBuffer);
	}

	entry = {};
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
		m_renderHardwareInterface->ReleaseOwnedResource(entry.scratchBuffer);
		entry.scratchBuffer = {};
		entry.scratchBufferSizeInBytes = 0;
	}
	if (entry.accelerationStructureBuffer && entry.accelerationStructureSizeInBytes < prebuildInfo.ResultDataMaxSizeInBytes)
	{
		m_renderHardwareInterface->ReleaseOwnedResource(entry.accelerationStructureBuffer);
		entry.accelerationStructureBuffer = {};
		entry.accelerationStructureSizeInBytes = 0;
	}

	if (!entry.scratchBuffer)
	{
		const std::uint64_t alignedScratchSize = AlignRayTracingBufferSize(
		    prebuildInfo.ScratchDataSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.ScratchBufferByteAlignment);
		entry.scratchBuffer = m_renderHardwareInterface->CreateRayTracingScratchBuffer(
		    alignedScratchSize,
		    L"RayTracingBlasScratch");
		entry.scratchBufferSizeInBytes = alignedScratchSize;
	}
	if (!entry.accelerationStructureBuffer)
	{
		const std::uint64_t alignedAccelerationStructureSize = AlignRayTracingBufferSize(
		    prebuildInfo.ResultDataMaxSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.AccelerationStructureByteAlignment);
		entry.accelerationStructureBuffer = m_renderHardwareInterface->CreateRayTracingAccelerationStructureBuffer(
		    alignedAccelerationStructureSize,
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

bool RayTracingBlasCache::GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return GeometryEquals(entry.geometry, geometry);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::BuildHandle(const Entry& entry) const noexcept
{
	if (m_renderHardwareInterface == nullptr || !entry.accelerationStructureBuffer)
	{
		return {};
	}

	return BlasHandle{
	    .resource = m_renderHardwareInterface->GetNativeResource(entry.accelerationStructureBuffer),
	    .gpuAddress = m_renderHardwareInterface->GetResourceGpuVirtualAddress(entry.accelerationStructureBuffer)};
}
