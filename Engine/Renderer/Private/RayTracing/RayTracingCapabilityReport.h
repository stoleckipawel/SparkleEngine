#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"

#include <cstdint>

struct RhiCapabilities;
struct RhiRayTracingCapabilities;

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
