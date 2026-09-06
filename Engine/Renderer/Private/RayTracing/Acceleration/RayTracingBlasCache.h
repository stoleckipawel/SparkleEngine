#pragma once

#include "Renderer/Public/Meshes/GpuMeshHandle.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstddef>
#include <DirectXMath.h>

#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <unordered_map>
#include <vector>

class GpuMesh;
class GpuMeshCache;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;
class RenderHardwareInterface;
struct MeshDraw;
struct PreparedRenderScene;

class RayTracingBlasCache final
{
public:
	struct BlasHandle final
	{
		RhiResourceHandle resource = {};
		RhiGpuVirtualAddress gpuAddress = 0;
		bool builtThisFrame = false;

		bool IsValid() const noexcept;
	};

	struct BuildStats final
	{
		std::uint32_t referencedMeshCount = 0;
		std::uint32_t builtBlasCount = 0;
		std::uint32_t reusedBlasCount = 0;
	};

	RayTracingBlasCache(RenderHardwareInterface& renderHardwareInterface, const GpuMeshCache& meshes) noexcept;
	~RayTracingBlasCache() noexcept;

	RayTracingBlasCache(const RayTracingBlasCache&) = delete;
	RayTracingBlasCache& operator=(const RayTracingBlasCache&) = delete;
	RayTracingBlasCache(RayTracingBlasCache&&) = delete;
	RayTracingBlasCache& operator=(RayTracingBlasCache&&) = delete;

	void BeginFrame() noexcept;
	BlasHandle EnsureBlas(
	    RenderCommandContext& commandContext,
	    const GpuMesh& gpuMesh,
	    RayTracingPerformanceDiagnostics* diagnostics = nullptr) noexcept;
	BlasHandle EnsureBlas(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
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

		bool operator==(const SkinnedEntryKey& other) const noexcept;
	};

	struct SkinnedEntryKeyHash final
	{
		std::size_t operator()(const SkinnedEntryKey& key) const noexcept;
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
	    RenderCommandContext& commandContext,
	    const RhiRayTracingGeometryDesc& geometry,
	    Entry& entry,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	BlasHandle EnsureSkinnedBlas(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    const MeshDraw& draw,
	    std::uint32_t gpuSceneSlot,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	RhiRayTracingGeometryDesc BuildSkinnedGeometry(const PreparedRenderScene& preparedScene, const MeshDraw& draw, Entry& entry) noexcept;
	void ReplaceDynamicVertexBuffer(std::span<const DirectX::XMFLOAT3> positions, Entry& entry) noexcept;
	RhiRayTracingGeometryDesc BuildSkinnedGeometryDesc(
	    const GpuMesh& gpuMesh,
	    const Entry& entry,
	    std::uint32_t vertexCount) const noexcept;
	void EnsureEntryResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo, Entry& entry) noexcept;
	void TrackBuildResources(RenderCommandContext& commandContext, const Entry& entry) const noexcept;
	bool GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept;
	BlasHandle BuildHandle(const Entry& entry) const noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	const GpuMeshCache* m_meshes = nullptr;
	std::map<GpuMeshHandle, Entry> m_entries;
	std::unordered_map<SkinnedEntryKey, Entry, SkinnedEntryKeyHash> m_skinnedEntries;
	std::vector<DirectX::XMFLOAT3> m_skinnedPositionScratch;
	BuildStats m_currentFrameStats = {};
};
