#pragma once

#include "RayTracing/Diagnostics/RayTracingPerformanceMetrics.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "RayTracing/RayTracingCapabilityReport.h"

#include <memory>

namespace DirectX
{
	struct XMFLOAT3;
}

class RenderCommandContext;
class RenderHardwareInterface;
class GpuMeshCache;
class PassExecutionDiagnostics;
class RayTracingBlasCache;
class RayTracingTopLevelAccelerationStructureStrategy;
class RayTracingTopLevelScenePlanner;
struct PreparedRenderScene;

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

	void PlanFrame(const PreparedRenderScene& preparedScene, const DirectX::XMFLOAT3& cameraPosition) noexcept;
	RenderRayTracingFrameBindings Prepare(const PreparedRenderScene& preparedScene) noexcept;
	void Build(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void Clear() noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.Core.SupportsRayTracing; }
	bool HasValidTlas() const noexcept;
	RhiGpuVirtualAddress GetTlasGpuAddress() const noexcept;
	const RayTracingCapabilityReport& GetCapabilities() const noexcept { return m_capabilityReport; }

private:
	void EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RayTracingCapabilityReport m_capabilityReport = {};
	RayTracingPerformanceMetrics m_performanceMetrics = {};
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> m_topLevelAccelerationStructureStrategy;
	std::unique_ptr<RayTracingTopLevelScenePlanner> m_topLevelScenePlanner;
	bool m_topLevelStrategyPrefersPartitionedTlas = false;
};
