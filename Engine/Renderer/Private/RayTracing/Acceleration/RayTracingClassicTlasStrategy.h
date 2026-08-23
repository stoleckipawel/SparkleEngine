#pragma once

#include "RayTracing/Acceleration/RayTracingClassicTlasBuilder.h"
#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"

class RenderHardwareInterface;

class RayTracingClassicTlasStrategy final : public RayTracingTopLevelAccelerationStructureStrategy
{
public:
	explicit RayTracingClassicTlasStrategy(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~RayTracingClassicTlasStrategy() noexcept override;

	const char* GetStrategyName() const noexcept override;
	ERhiRayTracingTopLevelProvider GetActiveProvider() const noexcept override;
	const char* GetActiveProviderReason() const noexcept override;
	RenderRayTracingFrameBindings Prepare(
	    const PreparedRenderScene& preparedScene,
	    const RayTracingPtlasPartitionPlan& viewPlan) noexcept override;
	RayTracingTopLevelAccelerationStructureBuildResult Build(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    RayTracingBlasCache& blasCache,
	    const RayTracingPtlasPartitionPlan& viewPlan,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept override;
	bool HasValidSceneTlas() const noexcept override;
	void Clear() noexcept override;

private:
	RayTracingClassicTlasBuilder m_classicTlasBuilder;
};
