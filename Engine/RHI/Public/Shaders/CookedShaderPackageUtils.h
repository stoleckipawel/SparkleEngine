#pragma once

#include "../RHIAPI.h"

#include "CookedShaderPackage.h"

#include <cstdint>
#include <string_view>

class PassParameterLayout;

struct ShaderPackageDefinition final
{
	const char* PackageId = nullptr;
	ShaderStageMask ExpectedStages = ShaderStageMask::None;
	CookedShaderPackageFeatureFlags RequiredFeatures = CookedShaderPackageFeatureFlags::None;

	bool IsValid() const noexcept { return PackageId != nullptr && PackageId[0] != '\0' && ExpectedStages != ShaderStageMask::None; }

	explicit operator bool() const noexcept { return IsValid(); }
};

SPARKLE_RHI_API std::uint64_t BuildShaderPackageKey(std::string_view packageId);
SPARKLE_RHI_API std::uint64_t BuildShaderBlobId(
	std::string_view packageId,
	std::string_view entryPoint,
	std::string_view exportName,
	std::string_view compilerBackendName,
	std::string_view codegenTarget,
	CookedShaderBinaryFormat binaryFormat);
SPARKLE_RHI_API std::uint64_t BuildPassParameterLayoutHash(const PassParameterLayout& layout);
SPARKLE_RHI_API const char* CookedShaderBinaryFormatToString(CookedShaderBinaryFormat format) noexcept;
