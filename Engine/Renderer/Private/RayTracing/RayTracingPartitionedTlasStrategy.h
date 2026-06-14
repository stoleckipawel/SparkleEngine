#pragma once

#include "RayTracing/RayTracingClassicTlasStrategy.h"
#include "RayTracing/RayTracingCapabilityReport.h"

class RenderHardwareInterface;

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
	RayTracingSceneFrameData Prepare(const RenderSceneData& sceneData) noexcept override;
	RayTracingTopLevelAccelerationStructureBuildResult Build(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingBlasCache& blasCache,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept override;
	void BuildPartitionedTlasLogicalUpdateResources(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept override;
	void PackPartitionedTlasNativeOperations(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept override;
	bool HasValidSceneTlas() const noexcept override;
	RhiOwnedResourceHandle GetSceneTlasResource() const noexcept override;
	RhiGpuVirtualAddress GetSceneTlasGpuAddress() const noexcept override;
	std::uint32_t GetSceneTlasInstanceCount() const noexcept override;
	void Clear() noexcept override;

  private:
	RayTracingClassicTlasStrategy m_classicFallbackStrategy;
};
