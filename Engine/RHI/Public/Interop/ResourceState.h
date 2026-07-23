#pragma once

#include "../RHIAPI.h"

#include <cstdint>

enum class ResourceState : std::uint8_t
{
	Undefined,
	Common,
	RenderTarget,
	DepthWrite,
	DepthRead,
	ShaderResource,
	UnorderedAccess,
	RayTracingAccelerationStructure,
	CopySource,
	CopyDest,
	Present,

	Count
};

SPARKLE_RHI_API const char* ResourceStateToString(ResourceState state) noexcept;
