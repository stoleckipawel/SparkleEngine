#include "PCH.h"
#include "SceneData/MaterialTextureTableCapability.h"

#include <algorithm>

namespace
{
	constexpr std::uint32_t kMaterialTextureTablePreferredFixedCapacity = 4096u;

	std::uint32_t ResolveShaderResourceDescriptorCapacity(const RhiBindingLimits& limits) noexcept
	{
		std::uint32_t capacity = limits.MaxShaderResourceDescriptors;
		if (limits.MaxDescriptorTableEntries != 0)
		{
			capacity = capacity == 0 ? limits.MaxDescriptorTableEntries : std::min(capacity, limits.MaxDescriptorTableEntries);
		}
		return capacity;
	}
}

MaterialTextureTableCapabilityReport BuildMaterialTextureTableCapabilityReport(
    const RhiCapabilities& capabilities) noexcept
{
	if (capabilities.BackendApi == ERhiBackendApi::Unknown || capabilities.DescriptorModel == ERhiDescriptorModel::Unknown)
	{
		return MaterialTextureTableCapabilityReport{
		    .StatusReason = "backend-descriptor-model-unknown"};
	}

	const std::uint32_t shaderResourceCapacity = ResolveShaderResourceDescriptorCapacity(capabilities.BindingLimits);
	if (shaderResourceCapacity == 0)
	{
		return MaterialTextureTableCapabilityReport{
		    .StatusReason = "shader-resource-descriptor-limit-unavailable"};
	}

	const std::uint32_t fixedCapacity = std::min(shaderResourceCapacity, kMaterialTextureTablePreferredFixedCapacity);
	return MaterialTextureTableCapabilityReport{
	    .Supported = true,
	    .SelectedPath = MaterialTextureTablePath::FixedCapacityDescriptorArray,
	    .SupportsRaytracingOnly = true,
	    .SupportsEverything = false,
	    .SupportsRuntimeSizedBindless = false,
	    .MaxTextureDescriptors = fixedCapacity,
	    .StatusReason = "fixed-capacity-descriptor-array-selected"};
}
