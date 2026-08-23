#pragma once

#include "Compiler/ShaderCompileRequest.h"
#include "Cooking/CookedStageBuild.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/ShaderMap.h"

#include <string>
#include <vector>

struct ShaderCookDesc final
{
	ShaderTypeId shaderTypeId = 0;
	std::string shaderTypeName;
	ShaderStage stage = ShaderStage::Count;
	std::string sourcePath;
	std::string entryPoint;
	ShaderFeatureFlags features = ShaderFeatureFlags::None;
	PassParameterLayout parameterLayout;
	ShaderParameterStructDescriptor parameterStruct;
};

struct ShaderCookProduct final
{
	ShaderTypeId shaderTypeId = 0;
	ShaderTarget target = kDefaultShaderTarget;
	ShaderFeatureFlags features = ShaderFeatureFlags::None;
	PassParameterLayout parameterLayout;
	std::vector<ShaderDescriptorBindingRemap> bindingRemaps;
	CookedStageBuild compiled;
};

using ShaderCookOutputSet = std::vector<ShaderCookProduct>;
