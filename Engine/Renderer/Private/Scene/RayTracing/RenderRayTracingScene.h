#pragma once

#include "RayTracing/Diagnostics/RayTracingPerformanceMetrics.h"
#include "RayTracing/RayTracingExecutionFrontend.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "Scene/RayTracing/RayTracingShaderTablePlan.h"

#include <cstdint>
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
	void SynchronizeShaderTablePlan(std::span<const RenderPrimitive> primitives, const RenderMaterialTable& materials) noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.SupportsAccelerationStructure; }
	bool HasValidTlas() const noexcept;
	RayTracingExecutionFrontend GetExecutionFrontend() const noexcept { return m_executionFrontend; }
	void BeginGraphBuild() noexcept
	{
		if (m_executionFrontend == RayTracingExecutionFrontend::Pipeline)
		{
			m_shaderTablePlan.BeginMaterializationSet();
		}
	}
	std::uint64_t GetGraphGeneration() const noexcept
	{
		return m_executionFrontend == RayTracingExecutionFrontend::Pipeline ? m_shaderTablePlan.GetGeneration() : 0u;
	}
	const RayTracingPerformanceMetrics& GetPerformanceMetrics() const noexcept { return m_performanceMetrics; }
	const RayTracingShaderTableMetrics& GetShaderTableMetrics() const noexcept { return m_shaderTablePlan.GetMetrics(); }
	RayTracingShaderTablePlan& GetShaderTablePlan() noexcept { return m_shaderTablePlan; }

private:
	RayTracingCapabilityReport m_capabilityReport = {};
	RayTracingExecutionFrontend m_executionFrontend = RayTracingExecutionFrontend::None;
	RayTracingPerformanceMetrics m_performanceMetrics = {};
	RayTracingShaderTablePlan m_shaderTablePlan;
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> m_topLevelAccelerationStructureStrategy;
};
