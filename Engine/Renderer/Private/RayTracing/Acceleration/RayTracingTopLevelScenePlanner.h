#pragma once

#include "RayTracing/Acceleration/RayTracingClassicTlasBuilder.h"

#include <cstdint>
#include <memory>

namespace DirectX
{
	struct XMFLOAT3;
}

class RayTracingBlasCache;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;
struct RayTracingPtlasPartitionPlan;
struct RenderSceneData;

class RayTracingTopLevelScenePlanner final
{
  public:
	RayTracingTopLevelScenePlanner() noexcept;
	~RayTracingTopLevelScenePlanner() noexcept;

	RayTracingTopLevelScenePlanner(const RayTracingTopLevelScenePlanner&) = delete;
	RayTracingTopLevelScenePlanner& operator=(const RayTracingTopLevelScenePlanner&) = delete;
	RayTracingTopLevelScenePlanner(RayTracingTopLevelScenePlanner&&) = delete;
	RayTracingTopLevelScenePlanner& operator=(RayTracingTopLevelScenePlanner&&) = delete;

	void PlanFrame(
	    const RenderSceneData& sceneData,
	    const DirectX::XMFLOAT3& cameraPosition) noexcept;
	const RayTracingPtlasPartitionPlan* GetCurrentPartitionPlan() const noexcept;
	RayTracingClassicTlasBuilder::BuildStats BuildClassicTlas(
	    RenderCommandContext& commandContext,
	    const RenderSceneData& sceneData,
	    RayTracingClassicTlasBuilder& classicTlasBuilder,
	    RayTracingBlasCache& blasCache,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept;
	void Clear() noexcept;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

