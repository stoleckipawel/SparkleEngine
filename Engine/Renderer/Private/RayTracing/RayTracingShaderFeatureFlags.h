#pragma once

#include "RHI/Public/Shaders/ShaderMap.h"

#include <cstdint>

namespace RayTracingShaderFeatureFlags
{
	inline constexpr ShaderFeatureFlags InlineRayQueryCore = static_cast<ShaderFeatureFlags>(
	    static_cast<std::uint32_t>(ShaderFeatureFlags::UsesInlineRayQuery)
	    | static_cast<std::uint32_t>(ShaderFeatureFlags::UsesAccelerationStructure));
	inline constexpr ShaderFeatureFlags InlineRayQuery = static_cast<ShaderFeatureFlags>(
	    static_cast<std::uint32_t>(InlineRayQueryCore) | static_cast<std::uint32_t>(ShaderFeatureFlags::UsesDescriptorIndexing));
	inline constexpr ShaderFeatureFlags InlineRayQueryFloat64 = static_cast<ShaderFeatureFlags>(
	    static_cast<std::uint32_t>(InlineRayQueryCore) | static_cast<std::uint32_t>(ShaderFeatureFlags::UsesFloat64));
	inline constexpr ShaderFeatureFlags SceneBindings = static_cast<ShaderFeatureFlags>(
	    static_cast<std::uint32_t>(ShaderFeatureFlags::UsesAccelerationStructure)
	    | static_cast<std::uint32_t>(ShaderFeatureFlags::UsesDescriptorIndexing));
}
