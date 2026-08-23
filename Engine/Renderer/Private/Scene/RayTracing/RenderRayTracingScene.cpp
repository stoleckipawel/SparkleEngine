#include "PCH.h"

#include "Scene/RayTracing/RenderRayTracingScene.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GpuMeshCache.h"
#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "Scene/Preparation/PreparedRenderScene.h"

static const auto g_renderRayTracingSceneLogger = Logging::GetOrCreateLogger("Renderer.RenderRayTracingScene");

RenderRayTracingScene::RenderRayTracingScene(
    RenderHardwareInterface& renderHardwareInterface,
    const GpuMeshCache& meshes,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_capabilityReport(capabilityReport)
{
	m_performanceMetrics.Providers.TopLevelProvider = m_capabilityReport.TopLevelProvider.SelectedProvider;
	m_performanceMetrics.Providers.TopLevelProviderReason = m_capabilityReport.TopLevelProvider.SelectionReason;
	m_performanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_performanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_performanceMetrics.Providers.PartitionedTlasCapabilityReason = m_capabilityReport.PartitionedTlas.CapabilityStatusReason;
	if (!m_capabilityReport.SupportsRayTracing)
	{
		return;
	}

	m_blasCache = std::make_unique<RayTracingBlasCache>(renderHardwareInterface, meshes);
	m_topLevelAccelerationStructureStrategy =
	    CreateRayTracingTopLevelAccelerationStructureStrategy(renderHardwareInterface, m_capabilityReport);
}

RenderRayTracingScene::~RenderRayTracingScene() noexcept = default;

RenderRayTracingFrameBindings RenderRayTracingScene::Prepare(
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlan& viewPlan) noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		Diagnostics::Fatal(
		    g_renderRayTracingSceneLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing scene has no top-level acceleration-structure strategy.");
	}
	return m_topLevelAccelerationStructureStrategy->Prepare(preparedScene, viewPlan);
}

void RenderRayTracingScene::Build(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlan& viewPlan,
    PassExecutionDiagnostics* diagnostics) noexcept
{
	RayTracingPerformanceDiagnostics performanceDiagnostics{diagnostics};

	if (m_blasCache == nullptr || m_topLevelAccelerationStructureStrategy == nullptr)
	{
		Diagnostics::Fatal(
		    g_renderRayTracingSceneLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing scene build has no BLAS cache or top-level strategy.");
	}

	m_blasCache->BeginFrame();
	const RayTracingTopLevelAccelerationStructureBuildResult topLevelBuild =
	    m_topLevelAccelerationStructureStrategy->Build(commandContext, preparedScene, *m_blasCache, viewPlan, &performanceDiagnostics);
	const RayTracingBlasCache::BuildStats blasStats = m_blasCache->EndFrame();

	m_performanceMetrics.Providers.TopLevelProvider = topLevelBuild.ActiveProvider;
	m_performanceMetrics.Providers.TopLevelProviderReason = topLevelBuild.ActiveProviderReason;
	m_performanceMetrics.Providers.PartitionedTlasProvider = m_capabilityReport.PartitionedTlas.Provider;
	m_performanceMetrics.Providers.SupportsPartitionedTlas = m_capabilityReport.PartitionedTlas.Supported;
	m_performanceMetrics.Providers.PartitionedTlasCapabilityReason = m_capabilityReport.PartitionedTlas.CapabilityStatusReason;
	m_performanceMetrics.Blas.ReferencedMeshCount = blasStats.referencedMeshCount;
	m_performanceMetrics.Blas.BuiltCount = blasStats.builtBlasCount;
	m_performanceMetrics.Blas.ReusedCount = blasStats.reusedBlasCount;
	m_performanceMetrics.TopLevelInstanceCount = topLevelBuild.InstanceCount;
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
}

bool RenderRayTracingScene::HasValidTlas() const noexcept
{
	return m_topLevelAccelerationStructureStrategy != nullptr && m_topLevelAccelerationStructureStrategy->HasValidSceneTlas();
}
