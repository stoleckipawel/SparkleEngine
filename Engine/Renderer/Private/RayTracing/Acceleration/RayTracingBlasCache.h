#pragma once

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <functional>
#include <unordered_map>

class GPUMesh;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;
struct MeshDraw;
struct RenderSceneData;

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
	BlasHandle EnsureBlas(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const MeshDraw& draw,
	    std::uint32_t renderInstanceIndex,
	    RayTracingPerformanceDiagnostics* diagnostics = nullptr) noexcept;
	BuildStats EndFrame() noexcept;
	void Clear() noexcept;

  private:
	struct SkinnedEntryKey final
	{
		const GPUMesh* Mesh = nullptr;
		std::uint32_t RenderInstanceIndex = 0u;

		bool operator==(const SkinnedEntryKey& other) const noexcept
		{
			return Mesh == other.Mesh && RenderInstanceIndex == other.RenderInstanceIndex;
		}
	};

	struct SkinnedEntryKeyHash final
	{
		std::size_t operator()(const SkinnedEntryKey& key) const noexcept
		{
			const std::size_t meshHash = std::hash<const GPUMesh*>{}(key.Mesh);
			const std::size_t instanceHash = std::hash<std::uint32_t>{}(key.RenderInstanceIndex);
			return meshHash ^ (instanceHash + 0x9e3779b9u + (meshHash << 6u) + (meshHash >> 2u));
		}
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
	    std::uint32_t renderInstanceIndex,
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
	bool GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept;
	BlasHandle BuildHandle(const Entry& entry) const noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::unordered_map<const GPUMesh*, Entry> m_entries;
	std::unordered_map<SkinnedEntryKey, Entry, SkinnedEntryKeyHash> m_skinnedEntries;
	BuildStats m_currentFrameStats = {};
};
