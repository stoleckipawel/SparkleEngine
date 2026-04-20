#pragma once

#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

inline constexpr std::string_view kShaderCookManifestFileName = "ShaderPackages.ini";

struct ShaderCookStageDesc final
{
	ShaderStage stage = ShaderStage::Count;
	std::filesystem::path sourcePath;
	std::string entryPoint;
};

struct ShaderCookPackageDesc final
{
	std::string packageId;
	std::string bindingLayoutId;
	std::string variantId = "Default";
	std::vector<ShaderCookStageDesc> stages;
};
