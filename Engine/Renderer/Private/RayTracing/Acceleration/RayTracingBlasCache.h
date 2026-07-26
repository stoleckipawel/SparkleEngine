#pragma once

#include "Renderer/Public/Meshes/GpuMeshHandle.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <functional>
#include <map>
#include <unordered_map>

class GPUMesh;
class GPUMeshCache;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;
class RenderHardwareInterface;
struct MeshDraw;
struct RenderSceneData;

class RayTracingBlasCache final
{
  public:
	struct BlasHandle final
	{
		RhiResourceHandle resource = {};
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

	RayTracingBlasCache(
	    RenderHardwareInterface& renderHardwareInterface,
	    const GPUMeshCache& meshes) noexcept;
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
	BlasHandle EnsureBlas(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const MeshDraw& draw,
	    std::uint32_t gpuSceneSlot,
	    RayTracingPerformanceDiagnostics* diagnostics = nullptr) noexcept;
	BuildStats EndFrame() noexcept;
	void Clear() noexcept;

  private:
	struct SkinnedEntryKey final
	{
		GpuMeshHandle Mesh;
		std::uint32_t GpuSceneSlot = 0u;

		bool operator==(
		    const SkinnedEntryKey& other) const noexcept;
	};

	struct SkinnedEntryKeyHash final
	{
		std::size_t operator()(
		    const SkinnedEntryKey& key) const noexcept;
	};

	struct Entry final
	{
		RhiRayTracingGeometryDesc geometry = {};
		RhiOwnedResourceHandle dynamicVertexBuffer = {};
		RhiVertexBufferView dynamicVertexBufferView = {};
		RhiOwnedResourceHandle scratchBuffer = {};
		RhiOwnedResourceHandle accelerationStructureBuffer = {};
		std::uint64_t scratchBufferSizeInBytes = 0;
		std::uint64_t accelerationStructureSizeInBytes = 0;
		bool touchedThisFrame = false;
	};

	void ReleaseEntryResources(Entry& entry) noexcept;
	BlasHandle BuildBlas(
	    RenderCommandContext& cmd,
	    const RhiRayTracingGeometryDesc& geometry,
	    Entry& entry,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	BlasHandle EnsureSkinnedBlas(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const MeshDraw& draw,
	    std::uint32_t gpuSceneSlot,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	bool BuildSkinnedGeometry(
	    const RenderSceneData& sceneData,
	    const MeshDraw& draw,
	    Entry& entry,
	    RhiRayTracingGeometryDesc& outGeometry) noexcept;
	bool EnsureEntryResources(
	    const RhiRayTracingGeometryDesc& geometry,
	    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
	    Entry& entry) noexcept;
	void TrackBuildResources(
	    RenderCommandContext& cmd,
	    const Entry& entry) const noexcept;
	bool GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept;
	BlasHandle BuildHandle(const Entry& entry) const noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	const GPUMeshCache* m_meshes = nullptr;
	std::map<GpuMeshHandle, Entry> m_entries;
	std::unordered_map<SkinnedEntryKey, Entry, SkinnedEntryKeyHash> m_skinnedEntries;
	BuildStats m_currentFrameStats = {};
};
