#pragma once

#include "RayTracing/RayTracingClassicTlasBuilder.h"
#include "RayTracing/RayTracingPtlasGpuUpdateMetrics.h"
#include "RayTracing/RayTracingSceneFramePlan.h"

#include <cstdint>
#include <memory>

class RayTracingBlasCache;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;
struct RenderSceneData;

struct RayTracingTopLevelScenePlannerMetrics final
{
	std::uint32_t PartitionCount = 0;
	std::uint32_t DirtyTransformCount = 0;
	std::uint32_t MovedPartitionCount = 0;
	std::uint32_t GlobalPartitionInstanceCount = 0;
	std::uint32_t DuplicateStableIndexCount = 0;
	bool Overflow = false;
	RayTracingPtlasGpuUpdateMetrics GpuUpdates;
};

class RayTracingTopLevelScenePlanner final
{
  public:
	RayTracingTopLevelScenePlanner() noexcept;
	~RayTracingTopLevelScenePlanner() noexcept;

	RayTracingTopLevelScenePlanner(const RayTracingTopLevelScenePlanner&) = delete;
	RayTracingTopLevelScenePlanner& operator=(const RayTracingTopLevelScenePlanner&) = delete;
	RayTracingTopLevelScenePlanner(RayTracingTopLevelScenePlanner&&) = delete;
	RayTracingTopLevelScenePlanner& operator=(RayTracingTopLevelScenePlanner&&) = delete;

	RayTracingSceneFramePlan PlanFrame(const RenderSceneData& sceneData) noexcept;
	RayTracingClassicTlasBuilder::BuildStats BuildClassicTlas(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingClassicTlasBuilder& classicTlasBuilder,
	    RayTracingBlasCache& blasCache,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	RayTracingTopLevelScenePlannerMetrics GetCurrentPlannerMetrics() const noexcept;
	void Clear() noexcept;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
