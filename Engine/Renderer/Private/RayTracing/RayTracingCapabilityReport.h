#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>

struct RhiCapabilities;

struct RayTracingCapabilityReport final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	bool SupportsRayTracing = false;
	bool SupportsInlineRayQuery = false;
	bool HasAccelerationStructureAlignment = false;
	bool HasScratchBufferAlignment = false;
	bool HasInstanceDescSize = false;
	std::uint32_t MaxTraceRecursionDepth = 0;
	std::uint32_t MaxRayPayloadSizeInBytes = 0;
	std::uint32_t MaxRayAttributeSizeInBytes = 0;
	std::uint64_t AccelerationStructureByteAlignment = 0;
	std::uint64_t ScratchBufferByteAlignment = 0;
	std::uint32_t InstanceDescSizeInBytes = 0;
	ERhiRayTracingTopLevelProvider SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	const char* SelectedTopLevelProviderReason = "not-queried";
	ERhiPartitionedTlasProvider PartitionedTlasProvider = ERhiPartitionedTlasProvider::None;
	bool SupportsPartitionedTlas = false;
	bool PartitionedTlasRequiresNvidiaDevice = false;
	bool PartitionedTlasRunsOnNvidiaDevice = false;
	bool SupportsVulkanNativePartitionedTlas = false;
	bool SupportsVulkanPartitionedTlasExtension = false;
	bool SupportsVulkanPartitionedTlasFeatureQuery = false;
	bool SupportsVulkanPartitionedTlasFunctionLoading = false;
	bool SupportsVulkanPartitionedTlasDescriptorPath = false;
	bool SupportsD3D12NvapiPartitionedTlas = false;
	bool SupportsD3D12NvapiPartitionedTlasHeaders = false;
	bool SupportsD3D12NvapiPartitionedTlasRuntime = false;
	bool SupportsD3D12PartitionedTlasDeviceInterface = false;
	bool SupportsD3D12PartitionedTlasCommandListInterface = false;
	bool SupportsD3D12PublicDxrPartitionedTlas = false;
	bool SupportsD3D12PublicDxrPartitionedTlasHeaders = false;
	bool SupportsGpuDrivenPartitionedTlasOperations = false;
	const char* PartitionedTlasCapabilityStatusReason = "not-queried";

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
