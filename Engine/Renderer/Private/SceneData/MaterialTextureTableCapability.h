#pragma once

#include "RHI/Public/Core/RhiCapabilities.h"
#include "SceneData/MaterialBindingMode.h"

#include <cstdint>

inline constexpr std::uint32_t MaterialTextureTableFixedCapacity = 4096u;

enum class MaterialTextureTablePath : std::uint8_t
{
	Unsupported = 0,
	FixedCapacityDescriptorArray,
	RuntimeSizedBindlessTable,
};

constexpr const char* MaterialTextureTablePathToString(MaterialTextureTablePath path) noexcept
{
	switch (path)
	{
		case MaterialTextureTablePath::FixedCapacityDescriptorArray:
			return "FixedCapacityDescriptorArray";
		case MaterialTextureTablePath::RuntimeSizedBindlessTable:
			return "RuntimeSizedBindlessTable";
		case MaterialTextureTablePath::Unsupported:
		default:
			return "Unsupported";
	}
}

struct MaterialTextureTableCapabilityReport final
{
	bool Supported = false;
	MaterialTextureTablePath SelectedPath = MaterialTextureTablePath::Unsupported;
	bool SupportsRuntimeSizedBindless = false;
	std::uint32_t SupportedMaterialBindingModeMask = 0u;
	std::uint32_t MaxTextureDescriptors = 0;
	const char* StatusReason = "not-queried";

	bool SupportsMaterialBindingMode(MaterialBindingMode mode) const noexcept
	{
		return (SupportedMaterialBindingModeMask & MaterialBindingModeMask(mode)) != 0u;
	}
};

MaterialTextureTableCapabilityReport BuildMaterialTextureTableCapabilityReport(
    const RhiCapabilities& capabilities) noexcept;
