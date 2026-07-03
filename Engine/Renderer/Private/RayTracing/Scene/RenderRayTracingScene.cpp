#include "PCH.h"

#include "RayTracing/Scene/RenderRayTracingScene.h"

#include "Commands/RenderCommandContext.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "RayTracing/Diagnostics/RayTracingSceneDiagnosticState.h"
#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "SceneData/RenderSceneData.h"

RenderRayTracingScene::RenderRayTracingScene(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface),
    m_capabilityReport(capabilityReport),
    m_diagnosticState(std::make_unique<RayTracingSceneDiagnosticState>())
{
	m_diagnosticState->PerformanceMetrics.Providers.TopLevelProvider = m_capabilityReport.TopLevelProvider.SelectedProvider;
	m_diagnosticState->PerformanceMetrics.Providers.TopLevelProviderReason = m_capabilityReport.TopLevelProvider.SelectionReason;
	m_diagnosticState->PerformanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_diagnosticState->PerformanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_diagnosticState->PerformanceMetrics.Providers.PartitionedTlasCapabilityReason = m_capabilityReport.PartitionedTlas.CapabilityStatusReason;
	m_topLevelScenePlanner = std::make_unique<RayTracingTopLevelScenePlanner>();

	if (!m_capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	m_blasCache = std::make_unique<RayTracingBlasCache>(renderHardwareInterface);
	m_topLevelStrategyPrefersPartitionedTlas =
	    CVarRayTracingPreferPartitionedTlas.Get() && m_capabilityReport.PartitionedTlas.Supported;
	m_topLevelAccelerationStructureStrategy =
	    CreateRayTracingTopLevelAccelerationStructureStrategy(renderHardwareInterface, m_capabilityReport);
}

RenderRayTracingScene::~RenderRayTracingScene() noexcept = default;

void RenderRayTracingScene::PlanFrame(
    const RenderSceneData& sceneData,
    const DirectX::XMFLOAT3& cameraPosition) noexcept
{
	EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode();
	if (m_topLevelScenePlanner != nullptr)
	{
		m_topLevelScenePlanner->PlanFrame(sceneData, cameraPosition, m_topLevelStrategyPrefersPartitionedTlas);
	}
}

RayTracingSceneFrameData RenderRayTracingScene::Prepare(const RenderSceneData& sceneData) noexcept
{

	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		return {};
	}
	EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode();

	const std::uint32_t estimatedInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	if (estimatedInstanceCount == 0)
	{
		m_topLevelAccelerationStructureStrategy->Clear();
		m_diagnosticState->PerformanceMetrics.Blas = {};
		m_diagnosticState->PerformanceMetrics.ClassicTlas = {};
		m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates = {};
		const RayTracingTopLevelScenePlannerMetrics plannerMetrics =
		    m_topLevelScenePlanner != nullptr ? m_topLevelScenePlanner->GetCurrentPlannerMetrics() : RayTracingTopLevelScenePlannerMetrics{};
		m_diagnosticState->PerformanceMetrics.PtlasPlanner = RayTracingPtlasPlannerMetrics{
		    .TotalRenderInstanceCount = plannerMetrics.TotalRenderInstanceCount,
		    .TraceableInstanceCount = plannerMetrics.TraceableInstanceCount,
		    .StaticTraceableInstanceCount = plannerMetrics.StaticTraceableInstanceCount,
		    .DynamicTraceableInstanceCount = plannerMetrics.DynamicTraceableInstanceCount,
		    .PartitionsPerAxis = plannerMetrics.PartitionsPerAxis,
		    .PartitionCount = plannerMetrics.PartitionCount,
		    .GridPartitionCount = plannerMetrics.GridPartitionCount,
		    .DirtyTransformCount = plannerMetrics.DirtyTransformCount,
		    .MovedPartitionCount = plannerMetrics.MovedPartitionCount,
		    .GlobalPartitionEligibleCount = plannerMetrics.GlobalPartitionEligibleCount,
		    .GlobalPartitionInstanceCount = plannerMetrics.GlobalPartitionInstanceCount,
		    .ActivePartitionCount = plannerMetrics.ActivePartitionCount,
		    .MaxPartitionActivityCount = plannerMetrics.MaxPartitionActivityCount,
		    .DuplicateStableIndexCount = plannerMetrics.DuplicateStableIndexCount,
		    .Overflow = plannerMetrics.Overflow};
		m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates = plannerMetrics.GpuUpdates;
		m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates.GpuDrivenOperationApiSupported =
		    m_capabilityReport.PartitionedTlas.SupportsGpuDrivenOperations;
		m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable =
		    m_capabilityReport.PartitionedTlas.SupportsGpuLogicalUpdateRecordWrites;
		m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates.FullGpuNativePackAvailable =
		    m_capabilityReport.PartitionedTlas.SupportsGpuNativeOperationPacking;
		return {};
	}

	return m_topLevelAccelerationStructureStrategy->Prepare(sceneData, m_topLevelScenePlanner.get());
}

void RenderRayTracingScene::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    PassExecutionDiagnostics* diagnostics) noexcept
{
	RayTracingPerformanceDiagnostics performanceDiagnostics{diagnostics};

	if (m_blasCache == nullptr || m_topLevelAccelerationStructureStrategy == nullptr)
	{
		return;
	}

	if (sceneData.meshInstances.empty())
	{
		return;
	}

	m_blasCache->BeginFrame();
	const RayTracingTopLevelAccelerationStructureBuildResult topLevelBuild =
	    m_topLevelAccelerationStructureStrategy->Build(
	        cmd,
	        sceneData,
	        *m_blasCache,
	        m_topLevelScenePlanner.get(),
	        &performanceDiagnostics);
	const RayTracingBlasCache::BuildStats blasStats = m_blasCache->EndFrame();
	const RayTracingTopLevelAccelerationStructureBuildStats& topLevelStats = topLevelBuild.Stats;

	m_diagnosticState->PerformanceMetrics.Providers.TopLevelProvider = topLevelBuild.ActiveProvider;
	m_diagnosticState->PerformanceMetrics.Providers.TopLevelProviderReason = topLevelBuild.ActiveProviderReason;
	m_diagnosticState->PerformanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_diagnosticState->PerformanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_diagnosticState->PerformanceMetrics.Providers.PartitionedTlasCapabilityReason = m_capabilityReport.PartitionedTlas.CapabilityStatusReason;
	m_diagnosticState->PerformanceMetrics.Blas.ReferencedMeshCount = blasStats.referencedMeshCount;
	m_diagnosticState->PerformanceMetrics.Blas.BuiltCount = blasStats.builtBlasCount;
	m_diagnosticState->PerformanceMetrics.Blas.ReusedCount = blasStats.reusedBlasCount;
	m_diagnosticState->PerformanceMetrics.ClassicTlas.CandidateInstanceCount = topLevelStats.Candidates.InstanceCount;
	m_diagnosticState->PerformanceMetrics.ClassicTlas.InstanceCount = topLevelStats.Build.InstanceCount;
	m_diagnosticState->PerformanceMetrics.ClassicTlas.MissingGpuMeshCount = topLevelStats.Candidates.MissingGpuMeshCount;
	m_diagnosticState->PerformanceMetrics.ClassicTlas.RejectedBlasCount = topLevelStats.Candidates.RejectedBlasCount;
	m_diagnosticState->PerformanceMetrics.ClassicTlas.Built = topLevelStats.Build.Built;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.TotalRenderInstanceCount = topLevelStats.PtlasPlanner.TotalRenderInstanceCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.TraceableInstanceCount = topLevelStats.PtlasPlanner.TraceableInstanceCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.StaticTraceableInstanceCount = topLevelStats.PtlasPlanner.StaticTraceableInstanceCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.DynamicTraceableInstanceCount = topLevelStats.PtlasPlanner.DynamicTraceableInstanceCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.PartitionsPerAxis = topLevelStats.PtlasPlanner.PartitionsPerAxis;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.PartitionCount = topLevelStats.PtlasPlanner.PartitionCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.GridPartitionCount = topLevelStats.PtlasPlanner.GridPartitionCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.DirtyTransformCount = topLevelStats.PtlasPlanner.DirtyTransformCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.MovedPartitionCount = topLevelStats.PtlasPlanner.MovedPartitionCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.GlobalPartitionEligibleCount = topLevelStats.PtlasPlanner.GlobalPartitionEligibleCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.GlobalPartitionInstanceCount = topLevelStats.PtlasPlanner.GlobalPartitionInstanceCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.ActivePartitionCount = topLevelStats.PtlasPlanner.ActivePartitionCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.MaxPartitionActivityCount = topLevelStats.PtlasPlanner.MaxPartitionActivityCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.DuplicateStableIndexCount = topLevelStats.PtlasPlanner.DuplicateStableIndexCount;
	m_diagnosticState->PerformanceMetrics.PtlasPlanner.Overflow = topLevelStats.PtlasPlanner.Overflow;
	const RayTracingTopLevelScenePlannerMetrics plannerMetrics =
	    m_topLevelScenePlanner != nullptr ? m_topLevelScenePlanner->GetCurrentPlannerMetrics() : RayTracingTopLevelScenePlannerMetrics{};
	m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates = plannerMetrics.GpuUpdates;
	if (topLevelBuild.ActiveProvider == ERhiRayTracingTopLevelProvider::PartitionedTlas)
	{
		m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates = topLevelBuild.PtlasGpuUpdates;
	}
	m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates.GpuDrivenOperationApiSupported =
	    m_capabilityReport.PartitionedTlas.SupportsGpuDrivenOperations;
	m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable =
	    m_capabilityReport.PartitionedTlas.SupportsGpuLogicalUpdateRecordWrites;
	m_diagnosticState->PerformanceMetrics.PtlasGpuUpdates.FullGpuNativePackAvailable =
	    m_capabilityReport.PartitionedTlas.SupportsGpuNativeOperationPacking;
}

void RenderRayTracingScene::BuildPartitionedTlasLogicalUpdateResources(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    PassExecutionDiagnostics* diagnostics) noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		return;
	}

	RayTracingPerformanceDiagnostics performanceDiagnostics{diagnostics};
	m_topLevelAccelerationStructureStrategy->BuildPartitionedTlasLogicalUpdateResources(
	    cmd,
	    sceneData,
	    m_topLevelScenePlanner.get(),
	    &performanceDiagnostics);
}

void RenderRayTracingScene::PackPartitionedTlasNativeOperations(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    PassExecutionDiagnostics* diagnostics) noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		return;
	}

	RayTracingPerformanceDiagnostics performanceDiagnostics{diagnostics};
	m_topLevelAccelerationStructureStrategy->PackPartitionedTlasNativeOperations(
	    cmd,
	    sceneData,
	    m_topLevelScenePlanner.get(),
	    &performanceDiagnostics);
}

void RenderRayTracingScene::Clear() noexcept
{
	if (m_topLevelAccelerationStructureStrategy != nullptr)
	{
		m_topLevelAccelerationStructureStrategy->Clear();
	}

	if (m_blasCache != nullptr)
	{
		m_blasCache->Clear();
	}
	if (m_topLevelScenePlanner != nullptr)
	{
		m_topLevelScenePlanner->Clear();
	}
}

bool RenderRayTracingScene::HasValidTlas() const noexcept
{
	return m_topLevelAccelerationStructureStrategy != nullptr &&
	       m_topLevelAccelerationStructureStrategy->HasValidSceneTlas();
}

RhiOwnedResourceHandle RenderRayTracingScene::GetTlasResource() const noexcept
{
	return m_topLevelAccelerationStructureStrategy != nullptr
	           ? m_topLevelAccelerationStructureStrategy->GetSceneTlasResource()
	           : RhiOwnedResourceHandle{};
}

RhiGpuVirtualAddress RenderRayTracingScene::GetTlasGpuAddress() const noexcept
{
	return m_topLevelAccelerationStructureStrategy != nullptr
	           ? m_topLevelAccelerationStructureStrategy->GetSceneTlasGpuAddress()
	           : 0;
}

RayTracingSceneTlasShaderAccessMode RenderRayTracingScene::GetTlasShaderAccessMode() const noexcept
{
	return m_topLevelAccelerationStructureStrategy != nullptr
	           ? m_topLevelAccelerationStructureStrategy->GetSceneTlasShaderAccessMode()
	           : RayTracingSceneTlasShaderAccessMode::Descriptor;
}

std::uint32_t RenderRayTracingScene::GetTlasInstanceCount() const noexcept
{
	return m_topLevelAccelerationStructureStrategy != nullptr
	           ? m_topLevelAccelerationStructureStrategy->GetSceneTlasInstanceCount()
	           : 0;
}

const RayTracingPerformanceMetrics& RenderRayTracingScene::GetPerformanceMetrics() const noexcept
{
	return m_diagnosticState->PerformanceMetrics;
}

void RenderRayTracingScene::EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode() noexcept
{
	if (m_renderHardwareInterface == nullptr || !m_capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	const bool wantsPartitionedTlas = CVarRayTracingPreferPartitionedTlas.Get() && m_capabilityReport.PartitionedTlas.Supported;
	if (m_topLevelAccelerationStructureStrategy != nullptr && wantsPartitionedTlas == m_topLevelStrategyPrefersPartitionedTlas)
	{
		return;
	}

	if (m_topLevelAccelerationStructureStrategy != nullptr)
	{
		m_topLevelAccelerationStructureStrategy->Clear();
	}
	if (m_topLevelScenePlanner != nullptr)
	{
		m_topLevelScenePlanner->Clear();
	}
	m_topLevelAccelerationStructureStrategy =
	    CreateRayTracingTopLevelAccelerationStructureStrategy(*m_renderHardwareInterface, m_capabilityReport);
	m_topLevelStrategyPrefersPartitionedTlas = wantsPartitionedTlas;
}



