#pragma once

#include "Frame/RayTracingSceneFrameData.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RayTracingPerformanceMetrics.h"
#include "RayTracing/RayTracingSceneDiagnostics.h"
#include "RayTracing/RayTracingSceneFramePlan.h"

#include <memory>

class RenderCommandContext;
class RenderHardwareInterface;
class PassExecutionDiagnostics;
class RayTracingBlasCache;
class RayTracingTopLevelAccelerationStructureStrategy;
class RayTracingTopLevelScenePlanner;
struct ResolvedGpuTiming;
struct RenderSceneData;

class RenderRayTracingScene final
{
  public:
	RenderRayTracingScene(RenderHardwareInterface& renderHardwareInterface, const RayTracingCapabilityReport& capabilityReport) noexcept;
	~RenderRayTracingScene() noexcept;

	RenderRayTracingScene(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene& operator=(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene(RenderRayTracingScene&&) = delete;
	RenderRayTracingScene& operator=(RenderRayTracingScene&&) = delete;

	RayTracingSceneFramePlan PlanFrame(const RenderSceneData& sceneData) noexcept;
	RayTracingSceneFrameData Prepare(const RenderSceneData& sceneData) noexcept;
	void Build(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void BuildPartitionedTlasLogicalUpdateResources(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void PackPartitionedTlasNativeOperations(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void Clear() noexcept;
	void BeginResolvedGpuTimingFrame() noexcept;
	void PublishResolvedGpuTiming(const ResolvedGpuTiming& timing) noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.Core.SupportsRayTracing; }
	bool HasValidTlas() const noexcept;
	RhiOwnedResourceHandle GetTlasResource() const noexcept;
	RhiGpuVirtualAddress GetTlasGpuAddress() const noexcept;
	std::uint32_t GetTlasInstanceCount() const noexcept;
	const RayTracingCapabilityReport& GetCapabilities() const noexcept { return m_capabilityReport; }
	const RayTracingPerformanceMetrics& GetPerformanceMetrics() const noexcept { return m_performanceMetrics; }

  private:
	RayTracingCapabilityReport m_capabilityReport = {};
	RayTracingPerformanceMetrics m_performanceMetrics = {};
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> m_topLevelAccelerationStructureStrategy;
	std::unique_ptr<RayTracingTopLevelScenePlanner> m_topLevelScenePlanner;
	RayTracingSceneDiagnostics m_diagnostics;
};
