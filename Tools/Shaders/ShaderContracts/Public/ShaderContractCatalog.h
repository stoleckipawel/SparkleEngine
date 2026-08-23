#pragma once

#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/Authoring/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/ShaderMap.h"

#include <string>
#include <vector>

struct ShaderContract final
{
	ShaderTypeId shaderTypeId = 0;
	std::string shaderName;
	std::string sourcePath;
	std::string entryPoint;
	ShaderStage stage = ShaderStage::Count;
	ShaderFeatureFlags features = ShaderFeatureFlags::None;
	ShaderParameterStructDescriptor parameterStruct;
	PassParameterLayout parameterLayout;
	bool hasParameterStruct = false;
};

using ShaderContractCatalog = std::vector<ShaderContract>;

struct ShaderContractVerificationFailure final
{
	std::string shaderName;
	std::string sourcePath;
	std::string entryPoint;
	ShaderStage stage = ShaderStage::Count;
	std::string parameterName;
	std::string expectedLayout;
	std::string reason;
};
