#pragma once

#include "../RHIAPI.h"

#include "CookedShaderPackage.h"

#include <cstdint>
#include <filesystem>
#include <string_view>

class PassParameterLayout;

struct ShaderPackageDefinition final
{
	const char* PackageId = nullptr;
	const char* VariantId = "Default";
	const char* BindingLayoutId = nullptr;
	ShaderStageMask ExpectedStages = ShaderStageMask::None;

	bool IsValid() const noexcept
	{
		return PackageId != nullptr && PackageId[0] != '\0' && VariantId != nullptr && VariantId[0] != '\0' &&
		       ExpectedStages != ShaderStageMask::None;
	}

	explicit operator bool() const noexcept { return IsValid(); }
};

SPARKLE_RHI_API std::uint64_t BuildShaderPackageKey(std::string_view packageId, std::string_view variantId = "Default");
SPARKLE_RHI_API std::uint64_t BuildShaderVariantHash(std::string_view variantId = "Default");
SPARKLE_RHI_API std::uint64_t BuildPassParameterLayoutHash(const PassParameterLayout& layout);
SPARKLE_RHI_API std::filesystem::path GetCookedShaderRootPath();
SPARKLE_RHI_API std::filesystem::path GetCookedShaderPackageRootPath();
SPARKLE_RHI_API std::filesystem::path GetCookedShaderRegistryPath();
SPARKLE_RHI_API std::filesystem::path BuildCookedShaderPackagePath(std::uint64_t packageKey);