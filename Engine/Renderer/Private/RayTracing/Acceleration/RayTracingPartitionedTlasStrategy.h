#pragma once

#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>

class RenderHardwareInterface;
struct MeshDraw;
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
	RayTracingSceneFrameData Prepare(const RenderSceneData& sceneData, RayTracingTopLevelScenePlanner* scenePlanner) noexcept override;
	RayTracingTopLevelAccelerationStructureBuildResult Build(
	    RenderCommandContext& commandContext,
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
	struct PartitionedBuildState;

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
	static bool IsUsablePartitionPlan(const RayTracingPtlasPartitionPlan* partitionPlan) noexcept;
	static std::uint32_t ResolveInstanceCapacity(const RenderSceneData& sceneData) noexcept;
	static std::uint32_t ResolvePartitionCount(const RayTracingPtlasPartitionPlan* partitionPlan) noexcept;
	static std::uint32_t ResolveMaxInstancesPerPartition(
	    std::uint32_t instanceCapacity,
	    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept;
	static bool CanUsePartitionedTlasProvider(const RayTracingCapabilityReport& capabilityReport) noexcept;
	static const char* ResolveInactiveProviderReason(const RayTracingCapabilityReport& capabilityReport) noexcept;
	static const char* ResolveActiveProviderReason() noexcept;
	static RayTracingSceneTlasShaderAccessMode ResolveActiveShaderAccessMode(const RayTracingCapabilityReport& capabilityReport) noexcept;
	static RhiPartitionedTlasInstanceFlags ResolveInstanceFlags(const RenderSceneData& sceneData, const MeshDraw& draw) noexcept;
	void EnsurePartitionedTlasResources(const RenderSceneData& sceneData, const RayTracingPtlasPartitionPlan* partitionPlan) noexcept;
	RhiPartitionedTlasDesc BuildPartitionedTlasLayout(const RenderSceneData& sceneData, const RayTracingPtlasPartitionPlan* partitionPlan)
	    const noexcept;
	RayTracingSceneFrameData BuildPartitionedTlasFrameData(const RenderSceneData& sceneData) const noexcept;
	RayTracingTopLevelAccelerationStructureBuildResult BuildPartitionedTlas(
	    RenderCommandContext& commandContext,
	    const RenderSceneData& sceneData,
	    RayTracingBlasCache& blasCache,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	static void CollectPartitionedInstances(
	    RenderCommandContext& commandContext,
	    const RenderSceneData& sceneData,
	    const RayTracingPtlasPartitionPlan* partitionPlan,
	    RayTracingBlasCache& blasCache,
	    RayTracingPerformanceDiagnostics* diagnostics,
	    PartitionedBuildState& state) noexcept;
	void PreparePartitionedOperationBuffer(PartitionedBuildState& state) noexcept;
	void RecordPartitionedBuild(
	    RenderCommandContext& commandContext,
	    const PartitionedBuildState& state,
	    RayTracingPerformanceDiagnostics* diagnostics) const noexcept;
	void TrackBuildResources(RenderCommandContext& commandContext) const noexcept;
	void ReleasePartitionedTlasResources() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RayTracingCapabilityReport m_capabilityReport = {};
	PartitionedTlasResources m_partitionedResources = {};
	const char* m_activeProviderReason = "partitioned-tlas-not-prepared";
};
