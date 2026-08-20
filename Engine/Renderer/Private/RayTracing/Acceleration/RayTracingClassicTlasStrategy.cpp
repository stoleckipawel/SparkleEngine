#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingClassicTlasStrategy.h"

#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "Scene/Preparation/PreparedRenderScene.h"

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
    const PreparedRenderScene& preparedScene,
    RayTracingTopLevelScenePlanner* scenePlanner) noexcept
{
	(void) scenePlanner;
	RayTracingSceneFrameData frameData{};
	const std::uint32_t estimatedInstanceCount =
	    static_cast<std::uint32_t>(preparedScene.rayTracingWork.ClassicTlasBlasInputIndices.size());
	m_classicTlasBuilder.Prepare(estimatedInstanceCount);

	frameData.TlasResource = m_classicTlasBuilder.GetTlas().resource;
	frameData.TlasGpuAddress = m_classicTlasBuilder.GetTlas().gpuAddress;
	frameData.TlasShaderAccessMode = m_shaderAccessMode;
	frameData.EstimatedInstanceCount = estimatedInstanceCount;
	return frameData;
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingClassicTlasStrategy::Build(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	RayTracingTopLevelAccelerationStructureBuildResult result{};
	result.ActiveProvider = GetActiveProvider();
	result.ActiveProviderReason = GetActiveProviderReason();
	result.Stats = scenePlanner != nullptr
	    ? scenePlanner->BuildClassicTlas(commandContext, preparedScene, m_classicTlasBuilder, blasCache, diagnostics)
	    : m_classicTlasBuilder.Build(commandContext, preparedScene, blasCache, diagnostics);
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
