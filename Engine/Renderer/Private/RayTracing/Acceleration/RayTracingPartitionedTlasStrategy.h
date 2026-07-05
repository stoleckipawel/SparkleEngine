#pragma once

#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"

#include <cstdint>

class RenderHardwareInterface;
struct RayTracingPtlasPartitionPlan;

class RayTracingPartitionedTlasStrategy final : public RayTracingTopLevelAccelerationStructureStrategy
{
  public:
	RayTracingPartitionedTlasStrategy(
	    RenderHardwareInterface& renderHardwareInterface,
	    const RayTracingCapabilityReport& capabilityReport) noexcept;
	~RayTracingPartitionedTlasStrategy() noexcept override;

	const char* GetStrategyName() const noexcept override;
	ERhiRayTracingTopLevelProvider GetActiveProvider() const noexcept override;
	const char* GetActiveProviderReason() const noexcept override;
	RayTracingSceneFrameData Prepare(
	    const RenderSceneData& sceneData,
	    RayTracingTopLevelScenePlanner* scenePlanner) noexcept override;
	RayTracingTopLevelAccelerationStructureBuildResult Build(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingBlasCache& blasCache,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept override;
	bool HasValidSceneTlas() const noexcept override;
	RhiOwnedResourceHandle GetSceneTlasResource() const noexcept override;
	RhiGpuVirtualAddress GetSceneTlasGpuAddress() const noexcept override;
	RayTracingSceneTlasShaderAccessMode GetSceneTlasShaderAccessMode() const noexcept override;
	std::uint32_t GetSceneTlasInstanceCount() const noexcept override;
	void Clear() noexcept override;

  private:
	struct PartitionedTlasResources final
	{
		RhiOwnedResourceHandle Storage = {};
		RhiOwnedResourceHandle Scratch = {};
		RhiOwnedResourceHandle NativeOperationData = {};
		RhiGpuVirtualAddress StorageAddress = 0;
		RhiGpuVirtualAddress ScratchAddress = 0;
		RhiGpuVirtualAddress NativeOperationDataAddress = 0;
		RhiPartitionedTlasDesc Layout = {};
		std::uint32_t InstanceCount = 0;
		bool Built = false;

		bool HasSceneTlas() const noexcept;
	};

	bool CanUseActivePartitionedTlasProvider() const noexcept;
	bool EnsurePartitionedTlasResources(const RenderSceneData& sceneData, const RayTracingPtlasPartitionPlan* partitionPlan) noexcept;
	RhiPartitionedTlasDesc BuildPartitionedTlasLayout(
	    const RenderSceneData& sceneData,
	    const RayTracingPtlasPartitionPlan* partitionPlan) const noexcept;
	RayTracingSceneFrameData BuildPartitionedTlasFrameData(const RenderSceneData& sceneData) const noexcept;
	RayTracingTopLevelAccelerationStructureBuildResult BuildPartitionedTlas(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingBlasCache& blasCache,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	void InvalidatePartitionedTlasSceneState() noexcept;
	void ReleasePartitionedTlasResources() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RayTracingCapabilityReport m_capabilityReport = {};
	PartitionedTlasResources m_partitionedResources = {};
	const char* m_activeProviderReason = "partitioned-tlas-not-prepared";
};
