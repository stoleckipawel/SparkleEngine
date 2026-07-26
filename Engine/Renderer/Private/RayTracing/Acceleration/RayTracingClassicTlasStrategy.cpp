#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingClassicTlasStrategy.h"

#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "SceneData/RenderSceneData.h"

RayTracingClassicTlasStrategy::RayTracingClassicTlasStrategy(
    RenderHardwareInterface& renderHardwareInterface,
    RayTracingSceneTlasShaderAccessMode shaderAccessMode) noexcept :
    m_classicTlasBuilder(renderHardwareInterface),
    m_shaderAccessMode(shaderAccessMode)
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

RayTracingSceneFrameData RayTracingClassicTlasStrategy::Prepare(
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner) noexcept
{
	(void)scenePlanner;
	RayTracingSceneFrameData frameData{};
	const std::uint32_t estimatedInstanceCount =
	    static_cast<std::uint32_t>(
	        sceneData.rayTracingWork
	            .ClassicTlasBlasInputIndices.size());
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
	frameData.TlasShaderAccessMode = m_shaderAccessMode;
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

RayTracingSceneTlasShaderAccessMode RayTracingClassicTlasStrategy::GetSceneTlasShaderAccessMode() const noexcept
{
	return m_shaderAccessMode;
}

std::uint32_t RayTracingClassicTlasStrategy::GetSceneTlasInstanceCount() const noexcept
{
	return m_classicTlasBuilder.GetTlas().instanceCount;
}

void RayTracingClassicTlasStrategy::Clear() noexcept
{
	m_classicTlasBuilder.Clear();
}
