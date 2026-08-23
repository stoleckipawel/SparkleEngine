#pragma once

#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "Scene/RayTracing/RayTracingSceneTlasShaderAccessMode.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <memory>

class RayTracingBlasCache;
class RayTracingPerformanceDiagnostics;
class RenderCommandContext;
class RenderHardwareInterface;
struct RayTracingCapabilityReport;
struct PreparedRenderScene;
struct RayTracingPtlasPartitionPlan;

struct RayTracingTopLevelAccelerationStructureBuildResult final
{
	std::uint32_t InstanceCount = 0;
	ERhiRayTracingTopLevelProvider ActiveProvider = ERhiRayTracingTopLevelProvider::None;
	const char* ActiveProviderReason = "not-built";
};

class RayTracingTopLevelAccelerationStructureStrategy
{
public:
	virtual ~RayTracingTopLevelAccelerationStructureStrategy() noexcept;

	RayTracingTopLevelAccelerationStructureStrategy(const RayTracingTopLevelAccelerationStructureStrategy&) = delete;
	RayTracingTopLevelAccelerationStructureStrategy& operator=(const RayTracingTopLevelAccelerationStructureStrategy&) = delete;
	RayTracingTopLevelAccelerationStructureStrategy(RayTracingTopLevelAccelerationStructureStrategy&&) = delete;
	RayTracingTopLevelAccelerationStructureStrategy& operator=(RayTracingTopLevelAccelerationStructureStrategy&&) = delete;

	virtual const char* GetStrategyName() const noexcept = 0;
	virtual ERhiRayTracingTopLevelProvider GetActiveProvider() const noexcept = 0;
	virtual const char* GetActiveProviderReason() const noexcept = 0;
	virtual RenderRayTracingFrameBindings Prepare(
	    const PreparedRenderScene& preparedScene,
	    const RayTracingPtlasPartitionPlan& viewPlan) noexcept = 0;
	virtual RayTracingTopLevelAccelerationStructureBuildResult Build(
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    RayTracingBlasCache& blasCache,
	    const RayTracingPtlasPartitionPlan& viewPlan,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept = 0;
	virtual bool HasValidSceneTlas() const noexcept = 0;
	virtual RhiOwnedResourceHandle GetSceneTlasResource() const noexcept = 0;
	virtual RhiGpuVirtualAddress GetSceneTlasGpuAddress() const noexcept = 0;
	virtual RayTracingSceneTlasShaderAccessMode GetSceneTlasShaderAccessMode() const noexcept = 0;
	virtual std::uint32_t GetSceneTlasInstanceCount() const noexcept = 0;
	virtual void Clear() noexcept = 0;

protected:
	RayTracingTopLevelAccelerationStructureStrategy() noexcept;
};

std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> CreateRayTracingTopLevelAccelerationStructureStrategy(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept;
