#include "PCH.h"

#include "RayTracing/Scene/RenderRayTracingScene.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GpuMeshCache.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "SceneData/RenderSceneData.h"

static const auto g_renderRayTracingSceneLogger = Logging::GetOrCreateLogger("Renderer.RenderRayTracingScene");

RenderRayTracingScene::RenderRayTracingScene(
    RenderHardwareInterface& renderHardwareInterface,
    const GpuMeshCache& meshes,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface), m_capabilityReport(capabilityReport)
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

	m_blasCache = std::make_unique<RayTracingBlasCache>(renderHardwareInterface, meshes);
	m_topLevelStrategyPrefersPartitionedTlas = CVarRayTracingPreferPartitionedTlas.Get();
	m_topLevelAccelerationStructureStrategy =
	    CreateRayTracingTopLevelAccelerationStructureStrategy(renderHardwareInterface, m_capabilityReport);
}

RenderRayTracingScene::~RenderRayTracingScene() noexcept = default;

void RenderRayTracingScene::PlanFrame(const RenderSceneData& sceneData, const DirectX::XMFLOAT3& cameraPosition) noexcept
{
	EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode();
	if (m_topLevelScenePlanner != nullptr)
	{
		m_topLevelScenePlanner->PlanFrame(sceneData, cameraPosition);
	}
}

RayTracingSceneFrameData RenderRayTracingScene::Prepare(const RenderSceneData& sceneData) noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		Diagnostics::Fatal(
		    g_renderRayTracingSceneLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing scene has no top-level acceleration-structure strategy.");
	}
	EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode();

	return m_topLevelAccelerationStructureStrategy->Prepare(sceneData, m_topLevelScenePlanner.get());
}

void RenderRayTracingScene::Build(
    RenderCommandContext& commandContext,
    const RenderSceneData& sceneData,
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
	    m_topLevelAccelerationStructureStrategy->Build(commandContext, sceneData, *m_blasCache, m_topLevelScenePlanner.get(), &performanceDiagnostics);
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
	m_performanceMetrics.ClassicTlas.InstanceCount = topLevelStats.InstanceCount;
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
	return m_topLevelAccelerationStructureStrategy != nullptr && m_topLevelAccelerationStructureStrategy->HasValidSceneTlas();
}

RhiOwnedResourceHandle RenderRayTracingScene::GetTlasResource() const noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		Diagnostics::Fatal(g_renderRayTracingSceneLogger, __FILE__, __LINE__, "Ray-tracing scene has no TLAS resource owner.");
	}
	return m_topLevelAccelerationStructureStrategy->GetSceneTlasResource();
}

RhiGpuVirtualAddress RenderRayTracingScene::GetTlasGpuAddress() const noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		Diagnostics::Fatal(g_renderRayTracingSceneLogger, __FILE__, __LINE__, "Ray-tracing scene has no TLAS address owner.");
	}
	return m_topLevelAccelerationStructureStrategy->GetSceneTlasGpuAddress();
}

RayTracingSceneTlasShaderAccessMode RenderRayTracingScene::GetTlasShaderAccessMode() const noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		Diagnostics::Fatal(g_renderRayTracingSceneLogger, __FILE__, __LINE__, "Ray-tracing scene has no TLAS shader-access owner.");
	}
	return m_topLevelAccelerationStructureStrategy->GetSceneTlasShaderAccessMode();
}

std::uint32_t RenderRayTracingScene::GetTlasInstanceCount() const noexcept
{
	if (m_topLevelAccelerationStructureStrategy == nullptr)
	{
		Diagnostics::Fatal(g_renderRayTracingSceneLogger, __FILE__, __LINE__, "Ray-tracing scene has no TLAS instance-count owner.");
	}
	return m_topLevelAccelerationStructureStrategy->GetSceneTlasInstanceCount();
}

const RayTracingPerformanceMetrics& RenderRayTracingScene::GetPerformanceMetrics() const noexcept
{
	return m_performanceMetrics;
}

void RenderRayTracingScene::EnsureTopLevelAccelerationStructureStrategyMatchesRuntimeMode() noexcept
{
	if (m_renderHardwareInterface == nullptr || !m_capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	const bool wantsPartitionedTlas = CVarRayTracingPreferPartitionedTlas.Get();
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
