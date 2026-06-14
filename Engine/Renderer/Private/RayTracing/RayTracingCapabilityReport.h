#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>

struct RhiCapabilities;

struct RayTracingCoreCapabilityReport final
{
	bool SupportsRayTracing = false;
	bool SupportsInlineRayQuery = false;
	std::uint32_t MaxTraceRecursionDepth = 0;
	std::uint32_t MaxRayPayloadSizeInBytes = 0;
	std::uint32_t MaxRayAttributeSizeInBytes = 0;
};

struct RayTracingAccelerationStructureCapabilityReport final
{
	bool HasAccelerationStructureAlignment = false;
	bool HasScratchBufferAlignment = false;
	bool HasInstanceDescSize = false;
	std::uint64_t AccelerationStructureByteAlignment = 0;
	std::uint64_t ScratchBufferByteAlignment = 0;
	std::uint32_t InstanceDescSizeInBytes = 0;
};

struct RayTracingTopLevelProviderCapabilityReport final
{
	ERhiRayTracingTopLevelProvider SelectedProvider = ERhiRayTracingTopLevelProvider::None;
	const char* SelectionReason = "not-queried";
};

struct RayTracingPartitionedTlasCapabilityReport final
{
	ERhiPartitionedTlasProvider Provider = ERhiPartitionedTlasProvider::None;
	bool Supported = false;
	bool RequiresNvidiaDevice = false;
	bool RunsOnNvidiaDevice = false;
	bool SupportsVulkanNativeProvider = false;
	bool SupportsVulkanExtension = false;
	bool SupportsVulkanFeatureQuery = false;
	bool SupportsVulkanFunctionLoading = false;
	bool SupportsVulkanDescriptorPath = false;
	bool SupportsD3D12NvapiProvider = false;
	bool SupportsD3D12NvapiHeaders = false;
	bool SupportsD3D12NvapiRuntime = false;
	bool SupportsD3D12DeviceInterface = false;
	bool SupportsD3D12CommandListInterface = false;
	bool SupportsD3D12PublicDxrProvider = false;
	bool SupportsD3D12PublicDxrHeaders = false;
	bool SupportsGpuDrivenOperations = false;
	const char* CapabilityStatusReason = "not-queried";
};

struct RayTracingCapabilityReport final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	RayTracingCoreCapabilityReport Core;
	RayTracingAccelerationStructureCapabilityReport AccelerationStructures;
	RayTracingTopLevelProviderCapabilityReport TopLevelProvider;
	RayTracingPartitionedTlasCapabilityReport PartitionedTlas;

	bool CanUseInlineRayQueryShadows() const noexcept;
	const char* GetInlineRayQueryShadowUnavailableReason() const noexcept;
};

class RayTracingCapabilityReporter final
{
  public:
	static RayTracingCapabilityReport Build(const RhiCapabilities& capabilities) noexcept;
	static void LogOnce(const RayTracingCapabilityReport& report) noexcept;

  private:
	static RayTracingCapabilityReport Build(ERhiBackendApi backendApi, const RhiRayTracingCapabilities& rayTracing) noexcept;
};
