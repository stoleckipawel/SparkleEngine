#include "PCH.h"

#include "Cooking/Cache/ShaderCacheKey.h"

#include "Compiler/ShaderCompileProfile.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Hash/HashUtils.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <format>

std::string ShaderCacheKey::ToHex() const
{
	return std::format("{:016X}", value);
}

ShaderCacheKey ShaderCacheKey::Compute(
	const ShaderCookPackageDesc& package,
	const ShaderCookStageDesc& stage,
	const ShaderCompileOptions& options,
	const std::uint64_t sourceHash,
	const std::uint64_t includeClosureHash,
	const std::uint64_t optionsHash,
	const std::string_view backendName,
	const std::uint64_t backendVersion)
{
	std::string canonical;
	canonical.reserve(256);
	canonical += std::string{kShaderCacheBackendVersion};
	canonical += '|';
	canonical += std::to_string(kCookedShaderPackageVersion);
	canonical += '|';
	canonical += std::to_string(kShaderCacheSchemaVersion);
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint64_t>(::BuildShaderPackageKey(package.packageId, package.variantId)));
	canonical += '|';
	canonical += package.variantId;
	canonical += '|';
	canonical += package.bindingLayoutId;
	canonical += '|';
	canonical += GetShaderStagePrefix(stage.stage);
	canonical += '|';
	canonical += ShaderCompileProfile::BuildTargetProfile(options);
	canonical += '|';
	canonical += std::to_string(sourceHash);
	canonical += '|';
	canonical += std::to_string(includeClosureHash);
	canonical += '|';
	canonical += std::to_string(optionsHash);
	canonical += '|';
	canonical += backendName;
	canonical += '|';
	canonical += std::to_string(backendVersion);

	ShaderCacheKey key;
	key.value = Hash::Fnv1a64(canonical);
	if (key.value == 0)
	{
		key.value = Hash::kFnv64OffsetBasis;
	}
	return key;
}
