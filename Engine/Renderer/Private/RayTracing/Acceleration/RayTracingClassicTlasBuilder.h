#pragma once

#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureBuildStats.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <vector>

class RenderCommandContext;
class RenderHardwareInterface;
class RayTracingPerformanceDiagnostics;
struct MeshDraw;
struct PreparedRenderScene;

class RayTracingClassicTlasBuilder final
{
public:
	using BuildStats = RayTracingTopLevelAccelerationStructureBuildStats;

	struct TlasHandle final
	{
		RhiOwnedResourceHandle resource = {};
		RhiGpuVirtualAddress gpuAddress = 0;
		std::uint32_t instanceCount = 0;

		bool IsValid() const noexcept;
	};

	explicit RayTracingClassicTlasBuilder(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~RayTracingClassicTlasBuilder() noexcept;

	RayTracingClassicTlasBuilder(const RayTracingClassicTlasBuilder&) = delete;
	RayTracingClassicTlasBuilder& operator=(const RayTracingClassicTlasBuilder&) = delete;
	RayTracingClassicTlasBuilder(RayTracingClassicTlasBuilder&&) = delete;
	RayTracingClassicTlasBuilder& operator=(RayTracingClassicTlasBuilder&&) = delete;

	void Prepare(std::uint32_t instanceCapacity) noexcept;
	BuildStats Build(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    RayTracingBlasCache& blasCache,
	    RayTracingPerformanceDiagnostics* diagnostics = nullptr) noexcept;
	const TlasHandle& GetTlas() const noexcept { return m_tlas; }
	void Clear() noexcept;

private:
	struct BuildState;

	static std::uint64_t AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept;
	static bool SupportsClassicTlasRefit(RenderHardwareInterface& renderHardwareInterface) noexcept;
	static ERhiClassicTlasBuildFlags ResolveClassicTlasBuildFlags(RenderHardwareInterface& renderHardwareInterface) noexcept;
	static std::uint64_t ResolveScratchSize(
	    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
	    ERhiClassicTlasBuildFlags buildFlags) noexcept;
	static RhiRayTracingInstanceFlags ResolveInstanceFlags(const PreparedRenderScene& preparedScene, const MeshDraw& draw) noexcept;
	static void CollectInstances(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    RayTracingBlasCache& blasCache,
	    RayTracingPerformanceDiagnostics* diagnostics,
	    BuildState& state) noexcept;
	void PrepareBuild(BuildState& state) noexcept;
	void RecordBuild(
	    RenderCommandContext& commandContext,
	    const BuildState& state,
	    RayTracingPerformanceDiagnostics* diagnostics) const noexcept;
	void TrackBuildResources(RenderCommandContext& commandContext) const noexcept;
	void ReleaseResources() noexcept;
	void EnsureResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo) noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_instanceBuffer = {};
	RhiOwnedResourceHandle m_scratchBuffer = {};
	RhiOwnedResourceHandle m_accelerationStructureBuffer = {};
	std::uint64_t m_scratchBufferSizeInBytes = 0;
	std::uint64_t m_accelerationStructureSizeInBytes = 0;
	bool m_resourcesAllowUpdate = false;
	TlasHandle m_tlas = {};
};
