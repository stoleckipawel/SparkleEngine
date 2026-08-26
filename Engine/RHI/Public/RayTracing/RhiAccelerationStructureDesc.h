#pragma once

#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiResourceHandles.h"

#include <cstdint>

inline constexpr std::uint32_t kRhiRayTracingMaxInstanceId = 0x00FFFFFFu;
inline constexpr std::uint32_t kRhiRayTracingMaxInstanceMask = 0x000000FFu;
inline constexpr std::uint32_t kRhiRayTracingMaxInstanceContributionToHitGroupIndex = 0x00FFFFFFu;

enum class ERhiRayTracingAccelerationStructureType : std::uint8_t
{
	BottomLevel,
	TopLevel,
};

struct RhiRayTracingBufferBinding
{
	RhiResourceHandle Resource = {};
	std::uint64_t OffsetInBytes = 0;
};

struct RhiRayTracingGeometryDesc
{
	RhiRayTracingBufferBinding VertexBuffer = {};
	std::uint32_t VertexStrideInBytes = 0;
	std::uint32_t VertexCount = 0;
	RhiRayTracingBufferBinding IndexBuffer = {};
	std::uint32_t IndexCount = 0;
	RhiIndexFormat IndexFormat = RhiIndexFormat::UInt32;
	bool Opaque = true;
};

struct RhiRayTracingAccelerationStructurePrebuildInfo
{
	std::uint64_t ResultDataMaxSizeInBytes = 0;
	std::uint64_t ScratchDataSizeInBytes = 0;
	std::uint64_t UpdateScratchDataSizeInBytes = 0;
};
