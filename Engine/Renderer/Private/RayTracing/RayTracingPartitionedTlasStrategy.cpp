#include "PCH.h"

#include "RayTracing/RayTracingPartitionedTlasStrategy.h"

#include "SceneData/RenderSceneData.h"

RayTracingPartitionedTlasStrategy::RayTracingPartitionedTlasStrategy(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_classicFallbackStrategy(renderHardwareInterface)
{
	(void)capabilityReport;
}

RayTracingPartitionedTlasStrategy::~RayTracingPartitionedTlasStrategy() noexcept = default;

const char* RayTracingPartitionedTlasStrategy::GetStrategyName() const noexcept
{
	return "PartitionedTlasStrategy";
}

ERhiRayTracingTopLevelProvider RayTracingPartitionedTlasStrategy::GetActiveProvider() const noexcept
{
	return ERhiRayTracingTopLevelProvider::ClassicTlas;
}

const char* RayTracingPartitionedTlasStrategy::GetActiveProviderReason() const noexcept
{
	return "partitioned-tlas-strategy-selected-classic-fallback-until-scene-tlas-resource-is-wired";
}

RayTracingSceneFrameData RayTracingPartitionedTlasStrategy::Prepare(const RenderSceneData& sceneData) noexcept
{
	return m_classicFallbackStrategy.Prepare(sceneData);
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingPartitionedTlasStrategy::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	RayTracingTopLevelAccelerationStructureBuildResult result =
	    m_classicFallbackStrategy.Build(cmd, sceneData, blasCache, scenePlanner, diagnostics);
	result.ActiveProvider = GetActiveProvider();
	result.ActiveProviderReason = GetActiveProviderReason();
	return result;
}

void RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLogicalUpdateResources(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	m_classicFallbackStrategy.BuildPartitionedTlasLogicalUpdateResources(cmd, sceneData, scenePlanner, diagnostics);
}

void RayTracingPartitionedTlasStrategy::PackPartitionedTlasNativeOperations(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	m_classicFallbackStrategy.PackPartitionedTlasNativeOperations(cmd, sceneData, scenePlanner, diagnostics);
}

bool RayTracingPartitionedTlasStrategy::HasValidSceneTlas() const noexcept
{
	return m_classicFallbackStrategy.HasValidSceneTlas();
}

RhiOwnedResourceHandle RayTracingPartitionedTlasStrategy::GetSceneTlasResource() const noexcept
{
	return m_classicFallbackStrategy.GetSceneTlasResource();
}

RhiGpuVirtualAddress RayTracingPartitionedTlasStrategy::GetSceneTlasGpuAddress() const noexcept
{
	return m_classicFallbackStrategy.GetSceneTlasGpuAddress();
}

std::uint32_t RayTracingPartitionedTlasStrategy::GetSceneTlasInstanceCount() const noexcept
{
	return m_classicFallbackStrategy.GetSceneTlasInstanceCount();
}

void RayTracingPartitionedTlasStrategy::Clear() noexcept
{
	m_classicFallbackStrategy.Clear();
}
