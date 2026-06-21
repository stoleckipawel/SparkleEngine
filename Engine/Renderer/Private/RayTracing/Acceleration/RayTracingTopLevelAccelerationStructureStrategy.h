#pragma once

#include "Frame/RayTracing/RayTracingSceneFrameData.h"
#include "RayTracing/Diagnostics/RayTracingPtlasGpuUpdateMetrics.h"
#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"
#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureBuildStats.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <memory>

class RayTracingBlasCache;
class RayTracingPerformanceDiagnostics;
class RayTracingTopLevelScenePlanner;
class RenderCommandContext;
class RenderHardwareInterface;
struct RayTracingCapabilityReport;
struct RenderSceneData;

struct RayTracingTopLevelAccelerationStructureBuildResult final
{
	RayTracingTopLevelAccelerationStructureBuildStats Stats;
	RayTracingPtlasGpuUpdateMetrics PtlasGpuUpdates;
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
	virtual RayTracingSceneFrameData Prepare(
	    const RenderSceneData& sceneData,
	    RayTracingTopLevelScenePlanner* scenePlanner) noexcept = 0;
	virtual RayTracingTopLevelAccelerationStructureBuildResult Build(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingBlasCache& blasCache,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept = 0;
	virtual void BuildPartitionedTlasLogicalUpdateResources(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingTopLevelScenePlanner* scenePlanner,
	    RayTracingPerformanceDiagnostics* diagnostics) noexcept = 0;
	virtual void PackPartitionedTlasNativeOperations(
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    RayTracingTopLevelScenePlanner* scenePlanner,
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

