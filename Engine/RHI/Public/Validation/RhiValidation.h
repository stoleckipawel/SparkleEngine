#pragma once

#include "../Bindings/RenderBindingSet.h"
#include "../Core/RhiCapabilities.h"
#include "../Resources/RhiResourceDesc.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <string_view>

namespace RhiValidation
{
	SPARKLE_RHI_API bool IsEnabled() noexcept;
	SPARKLE_RHI_API void ReportContractViolation(
	    std::string_view owner,
	    std::string_view condition,
	    std::string_view recommendedFix) noexcept;
	SPARKLE_RHI_API bool ValidateBindingSetDesc(
	    const RhiCapabilities& capabilities,
	    const RenderBindingSetDesc& desc,
	    std::string_view owner) noexcept;
	SPARKLE_RHI_API bool ValidateBindingSetDescriptorIndex(
	    std::uint32_t descriptorIndex,
	    std::uint32_t descriptorCount,
	    std::string_view owner) noexcept;
	SPARKLE_RHI_API bool ValidateTextureResourceDesc(
	    const RhiCapabilities& capabilities,
	    const RhiTextureResourceDesc& desc,
	    std::string_view owner) noexcept;
}