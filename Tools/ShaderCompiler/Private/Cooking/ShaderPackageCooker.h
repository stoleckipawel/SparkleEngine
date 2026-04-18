#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Engine::AssetAuthoring
{
	struct CookedShaderPackageOutput final
	{
		std::string packageId;
		std::string variantId;
		std::string bindingLayoutId;
		std::filesystem::path outputPath;
		std::uint64_t packageKey = 0;
		std::uint64_t sourceIdentityHash = 0;
		std::uint64_t bindingLayoutHash = 0;
		std::uint64_t variantHash = 0;
		ShaderStageMask declaredStages = ShaderStageMask::None;
	};

	struct ShaderPackageCookResult final
	{
		std::filesystem::path registryPath;
		std::vector<CookedShaderPackageOutput> packages;
		std::string errorMessage;

		bool Succeeded() const noexcept { return errorMessage.empty(); }
	};

	class ShaderPackageCooker final
	{
	  public:
		ShaderPackageCookResult CookAll() const;
	};
}