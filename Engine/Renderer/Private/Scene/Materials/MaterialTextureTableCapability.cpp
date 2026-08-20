#include "PCH.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"

#include <algorithm>

class MaterialTextureTableCapacity final
{
public:
	static std::uint32_t ResolveShaderResourceDescriptorCapacity(const RhiBindingLimits& limits) noexcept
	{
		std::uint32_t capacity = limits.MaxShaderResourceDescriptors;
		if (limits.MaxDescriptorTableEntries != 0)
		{
			capacity = capacity == 0 ? limits.MaxDescriptorTableEntries : std::min(capacity, limits.MaxDescriptorTableEntries);
		}
		return capacity;
	}
};

MaterialTextureTableCapabilityReport BuildMaterialTextureTableCapabilityReport(const RhiCapabilities& capabilities) noexcept
{
	if (capabilities.BackendApi == ERhiBackendApi::Unknown || capabilities.DescriptorModel == ERhiDescriptorModel::Unknown)
	{
		return MaterialTextureTableCapabilityReport{.StatusReason = "backend-descriptor-model-unknown"};
	}
	if (!capabilities.DescriptorIndexing.SupportsSampledImageArrayNonUniformIndexing)
	{
		return MaterialTextureTableCapabilityReport{.StatusReason = "sampled-image-array-non-uniform-indexing-unavailable"};
	}
	if (!capabilities.DescriptorIndexing.SupportsPartiallyBoundDescriptorArrays)
	{
		return MaterialTextureTableCapabilityReport{.StatusReason = "partially-bound-descriptor-arrays-unavailable"};
	}

	const std::uint32_t shaderResourceCapacity =
	    MaterialTextureTableCapacity::ResolveShaderResourceDescriptorCapacity(capabilities.BindingLimits);
	if (shaderResourceCapacity == 0)
	{
		return MaterialTextureTableCapabilityReport{.StatusReason = "shader-resource-descriptor-limit-unavailable"};
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
	    .SupportsRuntimeSizedBindless = false,
	    .SupportedMaterialBindingModeMask = MaterialBindingModeMask(MaterialBindingMode::RaytracingOnly),
	    .MaxTextureDescriptors = MaterialTextureTableFixedCapacity,
	    .StatusReason = "fixed-capacity-descriptor-array-selected"};
}
