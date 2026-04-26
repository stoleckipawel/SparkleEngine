#include "PCH.h"

#include "Compiler/ShaderCompileProfile.h"

#include "RHI/Public/Config/RenderConfig.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <string>

std::string ShaderCompileProfile::BuildTargetProfile(const ShaderCompileOptions& options)
{
	std::string profile;
	profile.reserve(8);
	profile += GetShaderStagePrefix(options.Stage);
	profile += '_';
	profile += std::to_string(RenderConfig::ShaderModelMajor);
	profile += '_';
	profile += std::to_string(RenderConfig::ShaderModelMinor);
	return profile;
}