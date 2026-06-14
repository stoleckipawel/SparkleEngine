#include "PCH.h"

#include "RayTracing/RenderRayTracingScene.h"

#include "Commands/RenderCommandContext.h"
#include "RayTracing/RayTracingPerformanceDiagnostics.h"
#include "SceneData/RenderSceneData.h"

RenderRayTracingScene::RenderRayTracingScene(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_capabilityReport(capabilityReport)
{
	m_performanceMetrics.TopLevelProvider = m_capabilityReport.SelectedTopLevelProvider;
	m_performanceMetrics.PartitionedTlasProvider = m_capabilityReport.PartitionedTlasProvider;
	m_performanceMetrics.SupportsPartitionedTlas = m_capabilityReport.SupportsPartitionedTlas;

	if (!m_capabilityReport.SupportsRayTracing)
	{
		return;
	}

	m_blasCache = std::make_unique<RayTracingBlasCache>(renderHardwareInterface);
	m_tlasBuilder = std::make_unique<RayTracingTlasBuilder>(renderHardwareInterface);
}

RayTracingSceneFrameData RenderRayTracingScene::Prepare(const RenderSceneData& sceneData) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RayTracing.ScenePrepare");
	m_performanceMetrics.ScenePrepareCpuMilliseconds = 0.0;
	RayTracingPerformanceDiagnostics diagnostics{m_performanceMetrics};
	auto cpuScope = diagnostics.BeginScenePrepareCpuScope();

	if (m_tlasBuilder == nullptr)
	{
		return {};
	}

	RayTracingSceneFrameData frameData{};
	const std::uint32_t estimatedInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	if (estimatedInstanceCount == 0)
	{
		if (m_tlasBuilder->GetTlas().IsValid())
		{
			frameData.IsAvailable = true;
			frameData.TlasResource = m_tlasBuilder->GetTlas().resource;
			frameData.TlasGpuAddress = m_tlasBuilder->GetTlas().gpuAddress;
		}
		m_performanceMetrics.CandidateInstanceCount = 0;
		m_performanceMetrics.TlasInstanceCount = m_tlasBuilder->GetTlas().instanceCount;
		return frameData;
	}

	if (!m_tlasBuilder->Prepare(estimatedInstanceCount))
	{
		return frameData;
	}

	frameData.IsAvailable = true;
	frameData.TlasResource = m_tlasBuilder->GetTlas().resource;
	frameData.TlasGpuAddress = m_tlasBuilder->GetTlas().gpuAddress;
	frameData.EstimatedInstanceCount = estimatedInstanceCount;
	return frameData;
}

void RenderRayTracingScene::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    PassExecutionDiagnostics* diagnostics) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RayTracing.SceneBuild");
	m_performanceMetrics.SceneBuildCpuMilliseconds = 0.0;
	m_performanceMetrics.BlasCpuMilliseconds = 0.0;
	m_performanceMetrics.TlasCpuMilliseconds = 0.0;
	m_performanceMetrics.TlasInstancePreparationCpuMilliseconds = 0.0;
	RayTracingPerformanceDiagnostics performanceDiagnostics{m_performanceMetrics, diagnostics};
	auto cpuScope = performanceDiagnostics.BeginSceneBuildCpuScope();

	if (m_blasCache == nullptr || m_tlasBuilder == nullptr)
	{
		return;
	}

	if (sceneData.meshInstances.empty())
	{
		return;
	}

	m_blasCache->BeginFrame();
	const RayTracingTlasBuilder::BuildStats tlasStats = m_tlasBuilder->Build(cmd, sceneData, *m_blasCache, &performanceDiagnostics);
	const RayTracingBlasCache::BuildStats blasStats = m_blasCache->EndFrame();

	m_performanceMetrics.TopLevelProvider = m_capabilityReport.SelectedTopLevelProvider;
	m_performanceMetrics.PartitionedTlasProvider = m_capabilityReport.PartitionedTlasProvider;
	m_performanceMetrics.SupportsPartitionedTlas = m_capabilityReport.SupportsPartitionedTlas;
	m_performanceMetrics.ReferencedMeshCount = blasStats.referencedMeshCount;
	m_performanceMetrics.BuiltBlasCount = blasStats.builtBlasCount;
	m_performanceMetrics.ReusedBlasCount = blasStats.reusedBlasCount;
	m_performanceMetrics.CandidateInstanceCount = tlasStats.candidateInstanceCount;
	m_performanceMetrics.TlasInstanceCount = tlasStats.instanceCount;
	m_performanceMetrics.MissingGpuMeshCount = tlasStats.missingGpuMeshCount;
	m_performanceMetrics.RejectedBlasCount = tlasStats.rejectedBlasCount;
	m_performanceMetrics.BuiltTlas = tlasStats.builtTlas;
	m_diagnostics.LogSceneUpdate(m_capabilityReport, blasStats, tlasStats);
}

void RenderRayTracingScene::Clear() noexcept
{
	if (m_tlasBuilder != nullptr)
	{
		m_tlasBuilder->Clear();
	}

	if (m_blasCache != nullptr)
	{
		m_blasCache->Clear();
	}
}

RhiOwnedResourceHandle RenderRayTracingScene::GetTlasResource() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().resource : RhiOwnedResourceHandle{};
}

RhiGpuVirtualAddress RenderRayTracingScene::GetTlasGpuAddress() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().gpuAddress : 0;
}

std::uint32_t RenderRayTracingScene::GetTlasInstanceCount() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().instanceCount : 0;
}

void RenderRayTracingScene::BeginResolvedGpuTimingFrame() noexcept
{
	RayTracingPerformanceDiagnostics::BeginResolvedGpuTimingFrame(m_performanceMetrics);
}

void RenderRayTracingScene::PublishResolvedGpuTiming(const ResolvedGpuTiming& timing) noexcept
{
	RayTracingPerformanceDiagnostics::PublishResolvedGpuTiming(m_performanceMetrics, timing);
}
