#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"
#include "SceneData/MaterialTextureTableCapability.h"

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
	bool SupportsDescriptor = false;
	bool SupportsShaderDeviceAddress = false;
	const char* CapabilityStatusReason = "not-queried";
};

struct RayTracingTlasShaderAccessCapabilityReport final
{
	bool SupportsDescriptor = false;
	bool SupportsShaderDeviceAddress = false;
};

struct RayTracingCapabilityReport final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	RayTracingCoreCapabilityReport Core;
	RayTracingAccelerationStructureCapabilityReport AccelerationStructures;
	RayTracingTopLevelProviderCapabilityReport TopLevelProvider;
	RayTracingPartitionedTlasCapabilityReport PartitionedTlas;
	RayTracingTlasShaderAccessCapabilityReport TlasShaderAccess;
	MaterialTextureTableCapabilityReport MaterialTextureTable;

	bool CanUseInlineRayQueryShadows() const noexcept;
	const char* GetInlineRayQueryShadowUnavailableReason() const noexcept;
};

class RayTracingCapabilityReporter final
{
  public:
	static RayTracingCapabilityReport Build(const RhiCapabilities& capabilities) noexcept;

  private:
	static RayTracingCapabilityReport BuildFromCapabilities(const RhiCapabilities& capabilities) noexcept;
};
