#pragma once

#include "../RHIAPI.h"

#include "../Resources/RenderConstantBufferData.h"
#include "../ShaderParameters/PassParameterLayout.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace ShaderPackageLayouts
{
	inline constexpr std::uint32_t kForwardOpaqueMaterialTextureCount = 5u;

	SPARKLE_RHI_API bool TryBuild(
	    std::string_view bindingLayoutId,
	    PassParameterLayout& outLayout,
	    std::string& outErrorMessage);
}