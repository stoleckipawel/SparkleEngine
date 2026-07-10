#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

namespace RayTracingShaderFeatureFlags
{
	inline constexpr CookedShaderPackageFeatureFlags DescriptorRayQuery = CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                                                                      CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                                                                      CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;
	inline constexpr CookedShaderPackageFeatureFlags DeviceAddressRayQuery =
	    DescriptorRayQuery | CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress;
}
