#pragma once

#include "../Resources/RhiResourceDesc.h"

#include <array>
#include <cstdint>

struct RhiRayTracingCapabilities
{
	bool SupportsRayTracing = false;
	bool SupportsInlineRayQuery = false;
};

struct RhiRayTracingGeometryDesc
{
	RhiGpuVirtualAddress VertexBuffer = 0;
	std::uint32_t VertexStrideInBytes = 0;
	std::uint32_t VertexCount = 0;
	RhiGpuVirtualAddress IndexBuffer = 0;
	std::uint32_t IndexCount = 0;
	RhiIndexFormat IndexFormat = RhiIndexFormat::UInt32;
	bool Opaque = true;
};

struct RhiRayTracingInstanceDesc
{
	std::array<float, 12> Transform = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
	std::uint32_t InstanceID = 0;
	std::uint32_t InstanceMask = 0xFF;
	std::uint32_t InstanceContributionToHitGroupIndex = 0;
	RhiGpuVirtualAddress AccelerationStructure = 0;
};

struct RhiRayTracingAccelerationStructurePrebuildInfo
{
	std::uint64_t ResultDataMaxSizeInBytes = 0;
	std::uint64_t ScratchDataSizeInBytes = 0;
	std::uint64_t UpdateScratchDataSizeInBytes = 0;
};
