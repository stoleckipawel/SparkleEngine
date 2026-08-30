#pragma once

#include "RHI/Public/Shaders/ShaderMap.h"

namespace RayTracingShaderFeatureFlags
{
	inline constexpr ShaderFeatureFlags InlineRayQuery = static_cast<ShaderFeatureFlags>(
	    static_cast<std::uint32_t>(ShaderFeatureFlags::UsesInlineRayQuery)
	    | static_cast<std::uint32_t>(ShaderFeatureFlags::UsesAccelerationStructure)
	    | static_cast<std::uint32_t>(ShaderFeatureFlags::UsesDescriptorIndexing));
	inline constexpr ShaderFeatureFlags SceneBindings = static_cast<ShaderFeatureFlags>(
	    static_cast<std::uint32_t>(ShaderFeatureFlags::UsesAccelerationStructure)
	    | static_cast<std::uint32_t>(ShaderFeatureFlags::UsesDescriptorIndexing));
}
