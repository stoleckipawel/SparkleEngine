#include "PCH.h"

#include "RayTracing/RenderRayTracingScene.h"

#include "Commands/RenderCommandContext.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/RayTracingTopLevelAccelerationStructureStrategy.h"
#include "RayTracing/RayTracingTopLevelScenePlanner.h"
#include "SceneData/RenderSceneData.h"

RenderRayTracingScene::RenderRayTracingScene(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface),
    m_capabilityReport(capabilityReport)
{
	m_performanceMetrics.Providers.TopLevelProvider = m_capabilityReport.TopLevelProvider.SelectedProvider;
	m_performanceMetrics.Providers.TopLevelProviderReason = m_capabilityReport.TopLevelProvider.SelectionReason;
	m_performanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_performanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_performanceMetrics.Providers.PartitionedTlasCapabilityReason = m_capabilityReport.PartitionedTlas.CapabilityStatusReason;
	m_topLevelScenePlanner = std::make_unique<RayTracingTopLevelScenePlanner>();

	if (!m_capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	m_blasCache = std::make_unique<RayTracingBlasCache>(renderHardwareInterface);
	m_topLevelStrategyPrefersPartitionedTlas =
	    CVarRhiRayTracingPreferPartitionedTlas.Get() && m_capabilityReport.PartitionedTlas.Supported;
	m_topLevelAccelerationStructureStrategy =
	    CreateRayTracingTopLevelAccelerationStructureStrategy(renderHardwareInterface, m_capabilityReport);
}

RenderRayTracingScene::~RenderRayTracingScene() noexcept = default;

RayTracingSceneFramePlan RenderRayTracingScene::PlanFrame(
    const RenderSceneData& sceneData,
    const DirectX::XMFLOAT3& cameraPosition) noexcept
{
	return m_topLevelScenePlanner != nullptr ? m_topLevelScenePlanner->PlanFrame(sceneData, cameraPosition) : RayTracingSceneFramePlan{};
}

RayTracingSceneFrameData RenderRayTracingScene::Prepare(const RenderSceneData& sceneData) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RayTracing.ScenePrepare");

	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		return {};
	}
	EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode();

	const std::uint32_t estimatedInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	if (estimatedInstanceCount == 0)
	{
		m_topLevelAccelerationStructureStrategy->Clear();
		m_performanceMetrics.Blas = {};
		m_performanceMetrics.ClassicTlas = {};
		m_performanceMetrics.PtlasGpuUpdates = {};
		const RayTracingTopLevelScenePlannerMetrics plannerMetrics =
		    m_topLevelScenePlanner != nullptr ? m_topLevelScenePlanner->GetCurrentPlannerMetrics() : RayTracingTopLevelScenePlannerMetrics{};
		m_performanceMetrics.PtlasPlanner = RayTracingPtlasPlannerMetrics{
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
		m_performanceMetrics.PtlasGpuUpdates = plannerMetrics.GpuUpdates;
		m_performanceMetrics.PtlasGpuUpdates.GpuDrivenOperationApiSupported =
		    m_capabilityReport.PartitionedTlas.SupportsGpuDrivenOperations;
		m_performanceMetrics.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable =
		    m_capabilityReport.PartitionedTlas.SupportsGpuLogicalUpdateRecordWrites;
		m_performanceMetrics.PtlasGpuUpdates.FullGpuNativePackAvailable =
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
	SPARKLE_CPU_SCOPE("Renderer.RayTracing.SceneBuild");
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

	m_performanceMetrics.Providers.TopLevelProvider = topLevelBuild.ActiveProvider;
	m_performanceMetrics.Providers.TopLevelProviderReason = topLevelBuild.ActiveProviderReason;
	m_performanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_performanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_performanceMetrics.Providers.PartitionedTlasCapabilityReason = m_capabilityReport.PartitionedTlas.CapabilityStatusReason;
	m_performanceMetrics.Blas.ReferencedMeshCount = blasStats.referencedMeshCount;
	m_performanceMetrics.Blas.BuiltCount = blasStats.builtBlasCount;
	m_performanceMetrics.Blas.ReusedCount = blasStats.reusedBlasCount;
	m_performanceMetrics.ClassicTlas.CandidateInstanceCount = topLevelStats.Candidates.InstanceCount;
	m_performanceMetrics.ClassicTlas.InstanceCount = topLevelStats.Build.InstanceCount;
	m_performanceMetrics.ClassicTlas.MissingGpuMeshCount = topLevelStats.Candidates.MissingGpuMeshCount;
	m_performanceMetrics.ClassicTlas.RejectedBlasCount = topLevelStats.Candidates.RejectedBlasCount;
	m_performanceMetrics.ClassicTlas.Built = topLevelStats.Build.Built;
	m_performanceMetrics.PtlasPlanner.TotalRenderInstanceCount = topLevelStats.PtlasPlanner.TotalRenderInstanceCount;
	m_performanceMetrics.PtlasPlanner.TraceableInstanceCount = topLevelStats.PtlasPlanner.TraceableInstanceCount;
	m_performanceMetrics.PtlasPlanner.StaticTraceableInstanceCount = topLevelStats.PtlasPlanner.StaticTraceableInstanceCount;
	m_performanceMetrics.PtlasPlanner.DynamicTraceableInstanceCount = topLevelStats.PtlasPlanner.DynamicTraceableInstanceCount;
	m_performanceMetrics.PtlasPlanner.PartitionsPerAxis = topLevelStats.PtlasPlanner.PartitionsPerAxis;
	m_performanceMetrics.PtlasPlanner.PartitionCount = topLevelStats.PtlasPlanner.PartitionCount;
	m_performanceMetrics.PtlasPlanner.GridPartitionCount = topLevelStats.PtlasPlanner.GridPartitionCount;
	m_performanceMetrics.PtlasPlanner.DirtyTransformCount = topLevelStats.PtlasPlanner.DirtyTransformCount;
	m_performanceMetrics.PtlasPlanner.MovedPartitionCount = topLevelStats.PtlasPlanner.MovedPartitionCount;
	m_performanceMetrics.PtlasPlanner.GlobalPartitionEligibleCount = topLevelStats.PtlasPlanner.GlobalPartitionEligibleCount;
	m_performanceMetrics.PtlasPlanner.GlobalPartitionInstanceCount = topLevelStats.PtlasPlanner.GlobalPartitionInstanceCount;
	m_performanceMetrics.PtlasPlanner.ActivePartitionCount = topLevelStats.PtlasPlanner.ActivePartitionCount;
	m_performanceMetrics.PtlasPlanner.MaxPartitionActivityCount = topLevelStats.PtlasPlanner.MaxPartitionActivityCount;
	m_performanceMetrics.PtlasPlanner.DuplicateStableIndexCount = topLevelStats.PtlasPlanner.DuplicateStableIndexCount;
	m_performanceMetrics.PtlasPlanner.Overflow = topLevelStats.PtlasPlanner.Overflow;
	const RayTracingTopLevelScenePlannerMetrics plannerMetrics =
	    m_topLevelScenePlanner != nullptr ? m_topLevelScenePlanner->GetCurrentPlannerMetrics() : RayTracingTopLevelScenePlannerMetrics{};
	m_performanceMetrics.PtlasGpuUpdates = plannerMetrics.GpuUpdates;
	if (topLevelBuild.ActiveProvider == ERhiRayTracingTopLevelProvider::PartitionedTlas)
	{
		m_performanceMetrics.PtlasGpuUpdates = topLevelBuild.PtlasGpuUpdates;
	}
	m_performanceMetrics.PtlasGpuUpdates.GpuDrivenOperationApiSupported =
	    m_capabilityReport.PartitionedTlas.SupportsGpuDrivenOperations;
	m_performanceMetrics.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable =
	    m_capabilityReport.PartitionedTlas.SupportsGpuLogicalUpdateRecordWrites;
	m_performanceMetrics.PtlasGpuUpdates.FullGpuNativePackAvailable =
	    m_capabilityReport.PartitionedTlas.SupportsGpuNativeOperationPacking;
	m_diagnostics.LogSceneUpdate(
	    m_capabilityReport,
	    topLevelBuild.ActiveProvider,
	    topLevelBuild.ActiveProviderReason,
	    blasStats,
	    topLevelStats);
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

void RenderRayTracingScene::EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode() noexcept
{
	if (m_renderHardwareInterface == nullptr || !m_capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	const bool wantsPartitionedTlas = CVarRhiRayTracingPreferPartitionedTlas.Get() && m_capabilityReport.PartitionedTlas.Supported;
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
