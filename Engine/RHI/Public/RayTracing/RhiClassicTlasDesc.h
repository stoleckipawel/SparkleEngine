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

enum class ERhiClassicTlasBuildFlags : std::uint8_t
{
	None = 0,
	AllowUpdate = 1 << 0,
};

enum class ERhiClassicTlasBuildMode : std::uint8_t
{
	Build,
	BuildAllowUpdate,
	Update,
};

enum class RhiRayTracingInstanceFlags : std::uint8_t
{
	None = 0,
	TriangleFacingCullDisable = 1 << 0,
	ForceOpaque = 1 << 1,
	ForceNonOpaque = 1 << 2,
};

constexpr ERhiClassicTlasBuildFlags operator|(ERhiClassicTlasBuildFlags lhs, ERhiClassicTlasBuildFlags rhs) noexcept
{
	return static_cast<ERhiClassicTlasBuildFlags>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

constexpr bool HasFlag(ERhiClassicTlasBuildFlags flags, ERhiClassicTlasBuildFlags flag) noexcept
{
	return (static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(flag)) != 0;
}

constexpr RhiRayTracingInstanceFlags operator|(RhiRayTracingInstanceFlags lhs, RhiRayTracingInstanceFlags rhs) noexcept
{
	return static_cast<RhiRayTracingInstanceFlags>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

constexpr bool HasFlag(RhiRayTracingInstanceFlags flags, RhiRayTracingInstanceFlags flag) noexcept
{
	return (static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(flag)) != 0;
}

struct RhiRayTracingInstanceDesc
{
	std::array<float, 12> Transform = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
	std::uint32_t InstanceID = 0;
	std::uint32_t InstanceMask = 0xFF;
	std::uint32_t InstanceContributionToHitGroupIndex = 0;
	RhiRayTracingInstanceFlags Flags = RhiRayTracingInstanceFlags::TriangleFacingCullDisable;
	RhiGpuVirtualAddress AccelerationStructure = 0;
};
