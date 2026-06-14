#pragma once

#include "Frame/RayTracingSceneFrameData.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RayTracingPerformanceMetrics.h"
#include "RayTracing/RayTracingSceneDiagnostics.h"
#include "RayTracing/RayTracingTlasBuilder.h"

#include <memory>

class RenderCommandContext;
class RenderHardwareInterface;
class PassExecutionDiagnostics;
struct ResolvedGpuTiming;
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

	RayTracingSceneFrameData Prepare(const RenderSceneData& sceneData) noexcept;
	void Build(RenderCommandContext& cmd, const RenderSceneData& sceneData, PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void Clear() noexcept;
	void BeginResolvedGpuTimingFrame() noexcept;
	void PublishResolvedGpuTiming(const ResolvedGpuTiming& timing) noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.SupportsRayTracing; }
	bool HasValidTlas() const noexcept { return m_tlasBuilder != nullptr && m_tlasBuilder->GetTlas().IsValid(); }
	RhiOwnedResourceHandle GetTlasResource() const noexcept;
	RhiGpuVirtualAddress GetTlasGpuAddress() const noexcept;
	std::uint32_t GetTlasInstanceCount() const noexcept;
	const RayTracingCapabilityReport& GetCapabilities() const noexcept { return m_capabilityReport; }
	const RayTracingPerformanceMetrics& GetPerformanceMetrics() const noexcept { return m_performanceMetrics; }

  private:
	RayTracingCapabilityReport m_capabilityReport = {};
	RayTracingPerformanceMetrics m_performanceMetrics = {};
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingTlasBuilder> m_tlasBuilder;
	RayTracingSceneDiagnostics m_diagnostics;
};
