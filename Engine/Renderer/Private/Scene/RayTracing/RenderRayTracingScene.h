#pragma once

#include "RayTracing/Diagnostics/RayTracingPerformanceMetrics.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "Scene/RayTracing/RayTracingShaderTablePlan.h"

#include <memory>
#include <span>

class RenderCommandContext;
class RenderHardwareInterface;
class GpuMeshCache;
class PassExecutionDiagnostics;
class RayTracingBlasCache;
class RayTracingTopLevelAccelerationStructureStrategy;
struct PreparedRenderScene;
struct RenderMaterialTable;
struct RenderPrimitive;
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
	void SynchronizeShaderTablePlan(
	    std::span<const RenderPrimitive> primitives,
	    const RenderMaterialTable& materials) noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.SupportsAccelerationStructure; }
	bool HasValidTlas() const noexcept;
	const RayTracingCapabilityReport& GetCapabilityReport() const noexcept { return m_capabilityReport; }
	const RayTracingPerformanceMetrics& GetPerformanceMetrics() const noexcept { return m_performanceMetrics; }
	const RayTracingShaderTableMetrics& GetShaderTableMetrics() const noexcept { return m_shaderTablePlan.GetMetrics(); }
	RayTracingShaderTablePlan& GetShaderTablePlan() noexcept { return m_shaderTablePlan; }
	const RayTracingShaderTablePlan& GetShaderTablePlan() const noexcept { return m_shaderTablePlan; }

private:
	RayTracingCapabilityReport m_capabilityReport = {};
	RayTracingPerformanceMetrics m_performanceMetrics = {};
	RayTracingShaderTablePlan m_shaderTablePlan;
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> m_topLevelAccelerationStructureStrategy;
};
