#include "PCH.h"

#include "RayTracing/RayTracingClassicTlasStrategy.h"

#include "RayTracing/RayTracingTopLevelScenePlanner.h"
#include "SceneData/RenderSceneData.h"

RayTracingClassicTlasStrategy::RayTracingClassicTlasStrategy(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_classicTlasBuilder(renderHardwareInterface)
{
}

RayTracingClassicTlasStrategy::~RayTracingClassicTlasStrategy() noexcept = default;

const char* RayTracingClassicTlasStrategy::GetStrategyName() const noexcept
{
	return "ClassicTlasStrategy";
}

ERhiRayTracingTopLevelProvider RayTracingClassicTlasStrategy::GetActiveProvider() const noexcept
{
	return ERhiRayTracingTopLevelProvider::ClassicTlas;
}

const char* RayTracingClassicTlasStrategy::GetActiveProviderReason() const noexcept
{
	return "classic-tlas-strategy-selected";
}

RayTracingSceneFrameData RayTracingClassicTlasStrategy::Prepare(const RenderSceneData& sceneData) noexcept
{
	RayTracingSceneFrameData frameData{};
	const std::uint32_t estimatedInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	if (estimatedInstanceCount == 0)
	{
		m_classicTlasBuilder.Clear();
		return frameData;
	}

	if (!m_classicTlasBuilder.Prepare(estimatedInstanceCount))
	{
		return frameData;
	}

	frameData.IsAvailable = true;
	frameData.TlasResource = m_classicTlasBuilder.GetTlas().resource;
	frameData.TlasGpuAddress = m_classicTlasBuilder.GetTlas().gpuAddress;
	frameData.EstimatedInstanceCount = estimatedInstanceCount;
	return frameData;
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingClassicTlasStrategy::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	RayTracingTopLevelAccelerationStructureBuildResult result{};
	result.ActiveProvider = GetActiveProvider();
	result.ActiveProviderReason = GetActiveProviderReason();
	result.Stats = scenePlanner != nullptr
	                   ? scenePlanner->BuildClassicTlas(cmd, sceneData, m_classicTlasBuilder, blasCache, diagnostics)
	                   : m_classicTlasBuilder.Build(cmd, sceneData, nullptr, blasCache, diagnostics);
	return result;
}

void RayTracingClassicTlasStrategy::BuildPartitionedTlasLogicalUpdateResources(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	(void)cmd;
	(void)sceneData;
	(void)scenePlanner;
	(void)diagnostics;
}

void RayTracingClassicTlasStrategy::PackPartitionedTlasNativeOperations(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	(void)cmd;
	(void)sceneData;
	(void)scenePlanner;
	(void)diagnostics;
}

bool RayTracingClassicTlasStrategy::HasValidSceneTlas() const noexcept
{
	return m_classicTlasBuilder.GetTlas().IsValid();
}

RhiOwnedResourceHandle RayTracingClassicTlasStrategy::GetSceneTlasResource() const noexcept
{
	return m_classicTlasBuilder.GetTlas().resource;
}

RhiGpuVirtualAddress RayTracingClassicTlasStrategy::GetSceneTlasGpuAddress() const noexcept
{
	return m_classicTlasBuilder.GetTlas().gpuAddress;
}

std::uint32_t RayTracingClassicTlasStrategy::GetSceneTlasInstanceCount() const noexcept
{
	return m_classicTlasBuilder.GetTlas().instanceCount;
}

void RayTracingClassicTlasStrategy::Clear() noexcept
{
	m_classicTlasBuilder.Clear();
}
