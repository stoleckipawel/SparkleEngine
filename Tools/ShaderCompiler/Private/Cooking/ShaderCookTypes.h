#pragma once

#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <filesystem>
#include <string>
#include <vector>

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
	PassParameterLayout bindingLayout;
	std::string variantId = "Default";
	std::vector<ShaderCookStageDesc> stages;
};