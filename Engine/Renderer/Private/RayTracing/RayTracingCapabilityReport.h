#pragma once

#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

struct RhiCapabilities;

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
	const char* CapabilityStatusReason = "not-queried";
};

struct RayTracingCapabilityReport final
{
	bool SupportsAccelerationStructure = false;
	bool SupportsInlineRayQuery = false;
	bool SupportsRayTracingPipeline = false;
	bool SupportsDescriptorIndexing = false;
	RayTracingTopLevelProviderCapabilityReport TopLevelProvider;
	RayTracingPartitionedTlasCapabilityReport PartitionedTlas;
};

RayTracingCapabilityReport BuildRayTracingCapabilityReport(const RhiCapabilities& capabilities) noexcept;
