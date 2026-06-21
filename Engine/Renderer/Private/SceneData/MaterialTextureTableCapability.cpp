#include "PCH.h"
#include "SceneData/MaterialTextureTableCapability.h"

#include <algorithm>

namespace
{
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
	if (capabilities.BackendApi == ERhiBackendApi::Vulkan)
	{
		return MaterialTextureTableCapabilityReport{
		    .StatusReason = "vulkan-descriptor-indexing-capability-not-reported"};
	}

	const std::uint32_t shaderResourceCapacity = ResolveShaderResourceDescriptorCapacity(capabilities.BindingLimits);
	if (shaderResourceCapacity == 0)
	{
		return MaterialTextureTableCapabilityReport{
		    .StatusReason = "shader-resource-descriptor-limit-unavailable"};
	}

	if (shaderResourceCapacity < MaterialTextureTableFixedCapacity)
	{
		return MaterialTextureTableCapabilityReport{
		    .MaxTextureDescriptors = shaderResourceCapacity,
		    .StatusReason = "shader-resource-descriptor-limit-below-material-texture-table-capacity"};
	}

	return MaterialTextureTableCapabilityReport{
	    .Supported = true,
	    .SelectedPath = MaterialTextureTablePath::FixedCapacityDescriptorArray,
	    .SupportsRaytracingOnly = true,
	    .SupportsEverything = false,
	    .SupportsRuntimeSizedBindless = false,
	    .MaxTextureDescriptors = MaterialTextureTableFixedCapacity,
	    .StatusReason = "fixed-capacity-descriptor-array-selected"};
}
