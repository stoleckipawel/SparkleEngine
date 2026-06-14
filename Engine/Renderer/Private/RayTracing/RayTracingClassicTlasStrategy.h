#pragma once

#include "RayTracing/RayTracingClassicTlasBuilder.h"
#include "RayTracing/RayTracingTopLevelAccelerationStructureStrategy.h"

class RenderHardwareInterface;

class RayTracingClassicTlasStrategy final : public RayTracingTopLevelAccelerationStructureStrategy
{
  public:
	explicit RayTracingClassicTlasStrategy(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~RayTracingClassicTlasStrategy() noexcept override;

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
	RayTracingClassicTlasBuilder m_classicTlasBuilder;
};
