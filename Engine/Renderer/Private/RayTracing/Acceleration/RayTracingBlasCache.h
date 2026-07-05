#pragma once

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <unordered_map>

class GPUMesh;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;

class RayTracingBlasCache final
{
  public:
	struct BlasHandle final
	{
		NativeResourceHandle resource = {};
		RhiGpuVirtualAddress gpuAddress = 0;
		bool builtThisFrame = false;

		bool IsValid() const noexcept { return resource && gpuAddress != 0; }
	};

	struct BuildStats final
	{
		std::uint32_t referencedMeshCount = 0;
		std::uint32_t builtBlasCount = 0;
		std::uint32_t reusedBlasCount = 0;
	};

	explicit RayTracingBlasCache(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~RayTracingBlasCache() noexcept;

	RayTracingBlasCache(const RayTracingBlasCache&) = delete;
	RayTracingBlasCache& operator=(const RayTracingBlasCache&) = delete;
	RayTracingBlasCache(RayTracingBlasCache&&) = delete;
	RayTracingBlasCache& operator=(RayTracingBlasCache&&) = delete;

	void BeginFrame() noexcept;
	BlasHandle EnsureBlas(
	    RenderCommandContext& cmd,
	    const GPUMesh& gpuMesh,
	    RayTracingPerformanceDiagnostics* diagnostics = nullptr) noexcept;
	BuildStats EndFrame() noexcept;
	void Clear() noexcept;

  private:
	struct Entry final
	{
		RhiRayTracingGeometryDesc geometry = {};
		RhiOwnedResourceHandle scratchBuffer = {};
		RhiOwnedResourceHandle accelerationStructureBuffer = {};
		std::uint64_t scratchBufferSizeInBytes = 0;
		std::uint64_t accelerationStructureSizeInBytes = 0;
		bool touchedThisFrame = false;
	};

	void ReleaseEntryResources(Entry& entry) noexcept;
	bool EnsureEntryResources(
	    const RhiRayTracingGeometryDesc& geometry,
	    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
	    Entry& entry) noexcept;
	bool GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept;
	BlasHandle BuildHandle(const Entry& entry) const noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::unordered_map<const GPUMesh*, Entry> m_entries;
	BuildStats m_currentFrameStats = {};
};
