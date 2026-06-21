#pragma once

#include "RayTracing/Acceleration/RayTracingClassicTlasBuilder.h"
#include "RayTracing/Diagnostics/RayTracingPtlasGpuUpdateMetrics.h"
#include "RayTracing/Scene/RayTracingSceneFramePlan.h"

#include <cstdint>
#include <memory>

namespace DirectX
{
	struct XMFLOAT3;
}

class RayTracingBlasCache;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;
struct RayTracingPtlasLogicalUpdateStreamResult;
struct RayTracingPtlasPartitionPlan;
struct RenderSceneData;

struct RayTracingTopLevelScenePlannerMetrics final
{
	std::uint32_t TotalRenderInstanceCount = 0;
	std::uint32_t TraceableInstanceCount = 0;
	std::uint32_t StaticTraceableInstanceCount = 0;
	std::uint32_t DynamicTraceableInstanceCount = 0;
	std::uint32_t PartitionsPerAxis = 0;
	std::uint32_t PartitionCount = 0;
	std::uint32_t GridPartitionCount = 0;
	std::uint32_t DirtyTransformCount = 0;
	std::uint32_t MovedPartitionCount = 0;
	std::uint32_t GlobalPartitionEligibleCount = 0;
	std::uint32_t GlobalPartitionInstanceCount = 0;
	std::uint32_t ActivePartitionCount = 0;
	std::uint32_t MaxPartitionActivityCount = 0;
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

	RayTracingSceneFramePlan PlanFrame(
	    const RenderSceneData& sceneData,
	    const DirectX::XMFLOAT3& cameraPosition,
	    bool buildPartitionedTlasUpdateStream) noexcept;
	const RayTracingPtlasPartitionPlan* GetCurrentPartitionPlan() const noexcept;
	const RayTracingPtlasLogicalUpdateStreamResult* GetCurrentLogicalUpdateStream() const noexcept;
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

