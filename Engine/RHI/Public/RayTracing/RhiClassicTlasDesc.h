#pragma once

#include "RhiAccelerationStructureDesc.h"

#include <array>
#include <cstdint>

struct RhiClassicTlasCapabilities
{
	bool SupportsClassicTlasBuild = false;
	bool SupportsClassicTlasUpdate = false;
	bool SupportsGpuReadableInstanceBuffer = false;
	std::uint32_t InstanceDescSizeInBytes = 0;
};

struct RhiRayTracingInstanceDesc
{
	std::array<float, 12> Transform = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
	std::uint32_t InstanceID = 0;
	std::uint32_t InstanceMask = 0xFF;
	std::uint32_t InstanceContributionToHitGroupIndex = 0;
	RhiGpuVirtualAddress AccelerationStructure = 0;
};
