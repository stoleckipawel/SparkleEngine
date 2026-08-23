#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

namespace RayTracingShaderFeatureFlags
{
	inline constexpr CookedShaderPackageFeatureFlags InlineRayQuery =
	    static_cast<CookedShaderPackageFeatureFlags>(
	        static_cast<std::uint32_t>(CookedShaderPackageFeatureFlags::UsesInlineRayQuery) |
	        static_cast<std::uint32_t>(CookedShaderPackageFeatureFlags::UsesAccelerationStructure) |
	        static_cast<std::uint32_t>(CookedShaderPackageFeatureFlags::UsesDescriptorIndexing));
}
