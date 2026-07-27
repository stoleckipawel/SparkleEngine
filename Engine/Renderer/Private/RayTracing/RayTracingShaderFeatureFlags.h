#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

namespace RayTracingShaderFeatureFlags
{
	inline constexpr CookedShaderPackageFeatureFlags DescriptorRayQuery =
	    static_cast<CookedShaderPackageFeatureFlags>(
	        static_cast<std::uint32_t>(CookedShaderPackageFeatureFlags::UsesInlineRayQuery) |
	        static_cast<std::uint32_t>(CookedShaderPackageFeatureFlags::UsesAccelerationStructure) |
	        static_cast<std::uint32_t>(CookedShaderPackageFeatureFlags::UsesDescriptorIndexing));
	inline constexpr CookedShaderPackageFeatureFlags DeviceAddressRayQuery =
	    static_cast<CookedShaderPackageFeatureFlags>(
	        static_cast<std::uint32_t>(DescriptorRayQuery) |
	        static_cast<std::uint32_t>(CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress));
}
