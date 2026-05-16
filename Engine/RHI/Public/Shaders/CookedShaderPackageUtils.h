#pragma once

#include "../RHIAPI.h"

#include "CookedShaderPackage.h"

#include <cstdint>
#include <string_view>

class PassParameterLayout;

struct ShaderPackageDefinition final
{
	const char* PackageId = nullptr;
	const char* BindingLayoutId = nullptr;
	ShaderStageMask ExpectedStages = ShaderStageMask::None;

	bool IsValid() const noexcept { return PackageId != nullptr && PackageId[0] != '\0' && ExpectedStages != ShaderStageMask::None; }

	explicit operator bool() const noexcept { return IsValid(); }
};

SPARKLE_RHI_API std::uint64_t BuildShaderPackageKey(std::string_view packageId);
SPARKLE_RHI_API std::uint64_t BuildPassParameterLayoutHash(const PassParameterLayout& layout);
SPARKLE_RHI_API const char* CookedShaderBinaryFormatToString(CookedShaderBinaryFormat format) noexcept;
