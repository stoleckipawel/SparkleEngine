#pragma once

#include "RayTracing/Diagnostics/RayTracingPerformanceMetrics.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "RayTracing/RayTracingCapabilityReport.h"

#include <memory>

class RenderCommandContext;
class RenderHardwareInterface;
class GpuMeshCache;
class PassExecutionDiagnostics;
class RayTracingBlasCache;
class RayTracingTopLevelAccelerationStructureStrategy;
struct PreparedRenderScene;
struct RayTracingPtlasPartitionPlan;

class RenderRayTracingScene final
{
public:
	RenderRayTracingScene(
	    RenderHardwareInterface& renderHardwareInterface,
	    const GpuMeshCache& meshes,
	    const RayTracingCapabilityReport& capabilityReport) noexcept;
	~RenderRayTracingScene() noexcept;

	RenderRayTracingScene(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene& operator=(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene(RenderRayTracingScene&&) = delete;
	RenderRayTracingScene& operator=(RenderRayTracingScene&&) = delete;

	RenderRayTracingFrameBindings Prepare(const PreparedRenderScene& preparedScene, const RayTracingPtlasPartitionPlan& viewPlan) noexcept;
	void Build(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    const RayTracingPtlasPartitionPlan& viewPlan,
	    PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void Clear() noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.Core.SupportsRayTracing; }
	bool CanUseInlineRayQueryShadows() const noexcept { return m_capabilityReport.CanUseInlineRayQueryShadows(); }
	const char* GetInlineRayQueryShadowUnavailableReason() const noexcept
	{
		return m_capabilityReport.GetInlineRayQueryShadowUnavailableReason();
	}
	bool HasValidTlas() const noexcept;

private:
	RayTracingCapabilityReport m_capabilityReport = {};
	RayTracingPerformanceMetrics m_performanceMetrics = {};
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> m_topLevelAccelerationStructureStrategy;
};
