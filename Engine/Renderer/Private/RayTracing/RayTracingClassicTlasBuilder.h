#pragma once

#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingTopLevelAccelerationStructureBuildStats.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <vector>

class RenderCommandContext;
class RayTracingPerformanceDiagnostics;
struct RayTracingPtlasPartitionPlan;
struct RenderSceneData;

class RayTracingClassicTlasBuilder final
{
  public:
	using BuildStats = RayTracingTopLevelAccelerationStructureBuildStats;

	struct TlasHandle final
	{
		RhiOwnedResourceHandle resource = {};
		RhiGpuVirtualAddress gpuAddress = 0;
		std::uint32_t instanceCount = 0;

		bool IsValid() const noexcept { return resource && gpuAddress != 0 && instanceCount > 0; }
	};

	explicit RayTracingClassicTlasBuilder(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~RayTracingClassicTlasBuilder() noexcept;

	RayTracingClassicTlasBuilder(const RayTracingClassicTlasBuilder&) = delete;
	RayTracingClassicTlasBuilder& operator=(const RayTracingClassicTlasBuilder&) = delete;
	RayTracingClassicTlasBuilder(RayTracingClassicTlasBuilder&&) = delete;
	RayTracingClassicTlasBuilder& operator=(RayTracingClassicTlasBuilder&&) = delete;

	bool Prepare(std::uint32_t instanceCapacity) noexcept;
	BuildStats Build(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const RayTracingPtlasPartitionPlan* partitionPlan,
	    RayTracingBlasCache& blasCache,
	    RayTracingPerformanceDiagnostics* diagnostics = nullptr) noexcept;
	const TlasHandle& GetTlas() const noexcept { return m_tlas; }
	void Clear() noexcept;

  private:
	static std::array<float, 12> BuildInstanceTransform(const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
	void ReleaseResources() noexcept;
	bool EnsureResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo) noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_instanceBuffer = {};
	RhiOwnedResourceHandle m_scratchBuffer = {};
	RhiOwnedResourceHandle m_accelerationStructureBuffer = {};
	std::uint64_t m_scratchBufferSizeInBytes = 0;
	std::uint64_t m_accelerationStructureSizeInBytes = 0;
	TlasHandle m_tlas = {};
};
