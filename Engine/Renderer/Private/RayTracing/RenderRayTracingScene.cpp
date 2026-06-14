#include "PCH.h"

#include "RayTracing/RenderRayTracingScene.h"

#include "Commands/RenderCommandContext.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/RayTracingTopLevelAccelerationStructureStrategy.h"
#include "RayTracing/RayTracingTopLevelScenePlanner.h"
#include "SceneData/RenderSceneData.h"

RenderRayTracingScene::RenderRayTracingScene(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_capabilityReport(capabilityReport)
{
	m_performanceMetrics.Providers.TopLevelProvider = m_capabilityReport.TopLevelProvider.SelectedProvider;
	m_performanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_performanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_topLevelScenePlanner = std::make_unique<RayTracingTopLevelScenePlanner>();

	if (!m_capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	m_blasCache = std::make_unique<RayTracingBlasCache>(renderHardwareInterface);
	m_topLevelAccelerationStructureStrategy =
	    CreateRayTracingTopLevelAccelerationStructureStrategy(renderHardwareInterface, m_capabilityReport);
}

RenderRayTracingScene::~RenderRayTracingScene() noexcept = default;

RayTracingSceneFramePlan RenderRayTracingScene::PlanFrame(const RenderSceneData& sceneData) noexcept
{
	return m_topLevelScenePlanner != nullptr ? m_topLevelScenePlanner->PlanFrame(sceneData) : RayTracingSceneFramePlan{};
}

RayTracingSceneFrameData RenderRayTracingScene::Prepare(const RenderSceneData& sceneData) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RayTracing.ScenePrepare");
	m_performanceMetrics.Timings.ScenePrepareCpuMilliseconds = 0.0;
	RayTracingPerformanceDiagnostics diagnostics{m_performanceMetrics};
	auto cpuScope = diagnostics.BeginScenePrepareCpuScope();

	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		return {};
	}

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
		    .PartitionCount = plannerMetrics.PartitionCount,
		    .DirtyTransformCount = plannerMetrics.DirtyTransformCount,
		    .MovedPartitionCount = plannerMetrics.MovedPartitionCount,
		    .GlobalPartitionInstanceCount = plannerMetrics.GlobalPartitionInstanceCount,
		    .DuplicateStableIndexCount = plannerMetrics.DuplicateStableIndexCount,
		    .Overflow = plannerMetrics.Overflow};
		m_performanceMetrics.PtlasGpuUpdates = plannerMetrics.GpuUpdates;
		m_performanceMetrics.PtlasGpuUpdates.FullGpuNativePackSupported =
		    m_capabilityReport.PartitionedTlas.SupportsGpuDrivenOperations;
		return {};
	}

	return m_topLevelAccelerationStructureStrategy->Prepare(sceneData);
}

void RenderRayTracingScene::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    PassExecutionDiagnostics* diagnostics) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RayTracing.SceneBuild");
	m_performanceMetrics.Timings.SceneBuildCpuMilliseconds = 0.0;
	m_performanceMetrics.Blas.CpuMilliseconds = 0.0;
	m_performanceMetrics.ClassicTlas.CpuMilliseconds = 0.0;
	m_performanceMetrics.ClassicTlas.InstancePreparationCpuMilliseconds = 0.0;
	RayTracingPerformanceDiagnostics performanceDiagnostics{m_performanceMetrics, diagnostics};
	auto cpuScope = performanceDiagnostics.BeginSceneBuildCpuScope();

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
	m_performanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_performanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_performanceMetrics.Blas.ReferencedMeshCount = blasStats.referencedMeshCount;
	m_performanceMetrics.Blas.BuiltCount = blasStats.builtBlasCount;
	m_performanceMetrics.Blas.ReusedCount = blasStats.reusedBlasCount;
	m_performanceMetrics.ClassicTlas.CandidateInstanceCount = topLevelStats.Candidates.InstanceCount;
	m_performanceMetrics.ClassicTlas.InstanceCount = topLevelStats.Build.InstanceCount;
	m_performanceMetrics.ClassicTlas.MissingGpuMeshCount = topLevelStats.Candidates.MissingGpuMeshCount;
	m_performanceMetrics.ClassicTlas.RejectedBlasCount = topLevelStats.Candidates.RejectedBlasCount;
	m_performanceMetrics.ClassicTlas.Built = topLevelStats.Build.Built;
	m_performanceMetrics.PtlasPlanner.PartitionCount = topLevelStats.PtlasPlanner.PartitionCount;
	m_performanceMetrics.PtlasPlanner.DirtyTransformCount = topLevelStats.PtlasPlanner.DirtyTransformCount;
	m_performanceMetrics.PtlasPlanner.MovedPartitionCount = topLevelStats.PtlasPlanner.MovedPartitionCount;
	m_performanceMetrics.PtlasPlanner.GlobalPartitionInstanceCount = topLevelStats.PtlasPlanner.GlobalPartitionInstanceCount;
	m_performanceMetrics.PtlasPlanner.DuplicateStableIndexCount = topLevelStats.PtlasPlanner.DuplicateStableIndexCount;
	m_performanceMetrics.PtlasPlanner.Overflow = topLevelStats.PtlasPlanner.Overflow;
	const RayTracingTopLevelScenePlannerMetrics plannerMetrics =
	    m_topLevelScenePlanner != nullptr ? m_topLevelScenePlanner->GetCurrentPlannerMetrics() : RayTracingTopLevelScenePlannerMetrics{};
	m_performanceMetrics.PtlasGpuUpdates = plannerMetrics.GpuUpdates;
	m_performanceMetrics.PtlasGpuUpdates.FullGpuNativePackSupported =
	    m_capabilityReport.PartitionedTlas.SupportsGpuDrivenOperations;
	m_diagnostics.LogSceneUpdate(
	    m_capabilityReport,
	    topLevelBuild.ActiveProvider,
	    topLevelBuild.ActiveProviderReason,
	    blasStats,
	    topLevelStats);
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

std::uint32_t RenderRayTracingScene::GetTlasInstanceCount() const noexcept
{
	return m_topLevelAccelerationStructureStrategy != nullptr
	           ? m_topLevelAccelerationStructureStrategy->GetSceneTlasInstanceCount()
	           : 0;
}

void RenderRayTracingScene::BeginResolvedGpuTimingFrame() noexcept
{
	RayTracingPerformanceDiagnostics::BeginResolvedGpuTimingFrame(m_performanceMetrics);
}

void RenderRayTracingScene::PublishResolvedGpuTiming(const ResolvedGpuTiming& timing) noexcept
{
	RayTracingPerformanceDiagnostics::PublishResolvedGpuTiming(m_performanceMetrics, timing);
}
