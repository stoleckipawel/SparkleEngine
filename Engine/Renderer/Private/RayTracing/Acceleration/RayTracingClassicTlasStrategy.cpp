#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingClassicTlasStrategy.h"

#include "Scene/Preparation/PreparedRenderScene.h"

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

RenderRayTracingFrameBindings RayTracingClassicTlasStrategy::Prepare(
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlan& viewPlan) noexcept
{
	(void) viewPlan;
	RenderRayTracingFrameBindings frameBindings{};
	const std::uint32_t estimatedInstanceCount =
	    static_cast<std::uint32_t>(preparedScene.rayTracingWork.ClassicTlasBlasInputIndices.size());
	m_classicTlasBuilder.Prepare(estimatedInstanceCount);

	frameBindings.TlasResource = m_classicTlasBuilder.GetTlas().resource;
	frameBindings.EstimatedInstanceCount = estimatedInstanceCount;
	return frameBindings;
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingClassicTlasStrategy::Build(
    RenderCommandContext& commandContext,
	const PreparedRenderScene& preparedScene,
	RayTracingBlasCache& blasCache,
	const RayTracingShaderTablePlan& shaderTablePlan,
	const RayTracingPtlasPartitionPlan& viewPlan,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	(void) viewPlan;
	RayTracingTopLevelAccelerationStructureBuildResult result{};
	result.ActiveProvider = GetActiveProvider();
	result.ActiveProviderReason = GetActiveProviderReason();
	result.InstanceCount = m_classicTlasBuilder.Build(commandContext, preparedScene, blasCache, shaderTablePlan, diagnostics);
	return result;
}

bool RayTracingClassicTlasStrategy::HasValidSceneTlas() const noexcept
{
	return m_classicTlasBuilder.GetTlas().IsValid();
}

void RayTracingClassicTlasStrategy::Clear() noexcept
{
	m_classicTlasBuilder.Clear();
}
