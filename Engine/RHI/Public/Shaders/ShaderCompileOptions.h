#pragma once

#include "Config/RenderConfig.h"
#include "ShaderStage.h"

#include <filesystem>
#include <string>
#include <vector>

struct ShaderCompileOptions
{
	std::filesystem::path SourcePath;
	std::filesystem::path IncludeDir;
	std::string EntryPoint = "main";
	ShaderStage Stage = ShaderStage::Pixel;

	bool EnableDebugInfo = false;
	bool EnableOptimizations = true;
	bool TreatWarningsAsErrors = true;
	bool StripReflection = true;
	bool StripDebugInfo = true;

	std::vector<std::filesystem::path> AdditionalIncludeDirs;

	std::vector<std::string> Defines;

	std::string BuildTargetProfile() const
	{
		std::string profile;
		profile.reserve(8);
		profile += GetShaderStagePrefix(Stage);
		profile += '_';
		profile += std::to_string(RenderConfig::ShaderModelMajor);
		profile += '_';
		profile += std::to_string(RenderConfig::ShaderModelMinor);
		return profile;
	}
};