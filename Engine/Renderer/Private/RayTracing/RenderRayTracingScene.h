#pragma once

#include "Frame/RayTracingSceneFrameData.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RayTracingClassicTlasBuilder.h"
#include "RayTracing/RayTracingPerformanceMetrics.h"
#include "RayTracing/RayTracingSceneDiagnostics.h"

#include <memory>

class RenderCommandContext;
class RenderHardwareInterface;
class PassExecutionDiagnostics;
struct ResolvedGpuTiming;
struct RayTracingPtlasPartitionPlan;
struct RenderSceneData;

class RenderRayTracingScene final
{
  public:
	RenderRayTracingScene(RenderHardwareInterface& renderHardwareInterface, const RayTracingCapabilityReport& capabilityReport) noexcept;
	~RenderRayTracingScene() noexcept = default;

	RenderRayTracingScene(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene& operator=(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene(RenderRayTracingScene&&) = delete;
	RenderRayTracingScene& operator=(RenderRayTracingScene&&) = delete;

	RayTracingSceneFrameData Prepare(const RenderSceneData& sceneData, const RayTracingPtlasPartitionPlan* partitionPlan = nullptr) noexcept;
	void Build(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const RayTracingPtlasPartitionPlan* partitionPlan = nullptr,
	    PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void Clear() noexcept;
	void BeginResolvedGpuTimingFrame() noexcept;
	void PublishResolvedGpuTiming(const ResolvedGpuTiming& timing) noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.Core.SupportsRayTracing; }
	bool HasValidTlas() const noexcept { return m_classicTlasBuilder != nullptr && m_classicTlasBuilder->GetTlas().IsValid(); }
	RhiOwnedResourceHandle GetTlasResource() const noexcept;
	RhiGpuVirtualAddress GetTlasGpuAddress() const noexcept;
	std::uint32_t GetTlasInstanceCount() const noexcept;
	const RayTracingCapabilityReport& GetCapabilities() const noexcept { return m_capabilityReport; }
	const RayTracingPerformanceMetrics& GetPerformanceMetrics() const noexcept { return m_performanceMetrics; }

  private:
	RayTracingCapabilityReport m_capabilityReport = {};
	RayTracingPerformanceMetrics m_performanceMetrics = {};
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingClassicTlasBuilder> m_classicTlasBuilder;
	RayTracingSceneDiagnostics m_diagnostics;
};
