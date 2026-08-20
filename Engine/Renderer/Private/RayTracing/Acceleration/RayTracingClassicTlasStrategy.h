#pragma once

#include "RayTracing/Acceleration/RayTracingClassicTlasBuilder.h"
#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"

class RenderHardwareInterface;

class RayTracingClassicTlasStrategy final : public RayTracingTopLevelAccelerationStructureStrategy
{
public:
	explicit RayTracingClassicTlasStrategy(
	    RenderHardwareInterface& renderHardwareInterface,
	    RayTracingSceneTlasShaderAccessMode shaderAccessMode = RayTracingSceneTlasShaderAccessMode::Descriptor) noexcept;
	~RayTracingClassicTlasStrategy() noexcept override;

	const char* GetStrategyName() const noexcept override;
	ERhiRayTracingTopLevelProvider GetActiveProvider() const noexcept override;
	const char* GetActiveProviderReason() const noexcept override;
	RayTracingSceneFrameData Prepare(
	    const PreparedRenderScene& preparedScene,
	    RayTracingTopLevelScenePlanner* scenePlanner) noexcept override;
	RayTracingTopLevelAccelerationStructureBuildResult Build(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    RayTracingBlasCache& blasCache,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept override;
	bool HasValidSceneTlas() const noexcept override;
	RhiOwnedResourceHandle GetSceneTlasResource() const noexcept override;
	RhiGpuVirtualAddress GetSceneTlasGpuAddress() const noexcept override;
	RayTracingSceneTlasShaderAccessMode GetSceneTlasShaderAccessMode() const noexcept override;
	std::uint32_t GetSceneTlasInstanceCount() const noexcept override;
	void Clear() noexcept override;

private:
	RayTracingClassicTlasBuilder m_classicTlasBuilder;
	RayTracingSceneTlasShaderAccessMode m_shaderAccessMode = RayTracingSceneTlasShaderAccessMode::Descriptor;
};
