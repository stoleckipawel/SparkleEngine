#pragma once

#include "RayTracing/Diagnostics/RayTracingPerformanceMetrics.h"
#include "RayTracing/Scene/RayTracingSceneFrameData.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"

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
struct RenderSceneData;

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

	void PlanFrame(const RenderSceneData& sceneData, const DirectX::XMFLOAT3& cameraPosition) noexcept;
	RayTracingSceneFrameData Prepare(const RenderSceneData& sceneData) noexcept;
	void Build(
	    RenderCommandContext& commandContext,
	    const RenderSceneData& sceneData,
	    PassExecutionDiagnostics* diagnostics = nullptr) noexcept;
	void Clear() noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.Core.SupportsRayTracing; }
	bool HasValidTlas() const noexcept;
	RhiOwnedResourceHandle GetTlasResource() const noexcept;
	RhiGpuVirtualAddress GetTlasGpuAddress() const noexcept;
	RayTracingSceneTlasShaderAccessMode GetTlasShaderAccessMode() const noexcept;
	std::uint32_t GetTlasInstanceCount() const noexcept;
	const RayTracingCapabilityReport& GetCapabilities() const noexcept { return m_capabilityReport; }
	const RayTracingPerformanceMetrics& GetPerformanceMetrics() const noexcept;

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
