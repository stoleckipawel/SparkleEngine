#pragma once

#include "RhiAccelerationStructureDesc.h"
#include "RhiClassicTlasDesc.h"
#include "RhiPartitionedTlasDesc.h"
#include "../RHIAPI.h"

#include <cstdint>

inline constexpr std::uint32_t kRhiRayTracingMaxPayloadSizeInBytes = 128;

enum class ERhiRayTracingTopLevelProvider : std::uint8_t
{
	None,
	ClassicTlas,
	PartitionedTlas,
};

struct RhiRayTracingProviderCapabilities
{
	ERhiRayTracingTopLevelProvider SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	const char* SelectedTopLevelProviderReason = "not-queried";
};

struct RhiRayTracingCapabilityGroups
{
	RhiClassicTlasCapabilities ClassicTlas;
	RhiPartitionedTlasCapabilities PartitionedTlas;
	RhiRayTracingProviderCapabilities Provider;
};

struct RhiRayTracingCapabilities
{
	bool SupportsAccelerationStructure = false;
	bool SupportsInlineRayQuery = false;
	bool SupportsRayTracingPipeline = false;
	std::uint32_t MaxTraceRecursionDepth = 0;
	std::uint32_t MaxRayPayloadSizeInBytes = 0;
	std::uint32_t MaxRayAttributeSizeInBytes = 0;
	std::uint32_t ShaderGroupHandleSizeInBytes = 0;
	std::uint32_t ShaderTableAlignmentInBytes = 0;
	std::uint32_t ShaderTableRecordAlignmentInBytes = 0;
	std::uint32_t MaxShaderTableRecordStrideInBytes = 0;
	std::uint64_t AccelerationStructureByteAlignment = 0;
	std::uint64_t ScratchBufferByteAlignment = 0;
	std::uint32_t InstanceDescSizeInBytes = 0;
	RhiRayTracingCapabilityGroups Groups;
};

SPARKLE_RHI_API const char* RhiRayTracingTopLevelProviderToString(ERhiRayTracingTopLevelProvider provider) noexcept;
SPARKLE_RHI_API void PopulateStandardRayTracingCapabilityGroups(RhiRayTracingCapabilities& capabilities) noexcept;
