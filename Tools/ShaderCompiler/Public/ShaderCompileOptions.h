#pragma once

#include "Backend/ShaderTarget.h"
#include "RHI/Public/Config/RenderConfig.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <filesystem>
#include <string>
#include <vector>

// Inputs to one shader compile invocation. Owned by the offline tool; runtime
// modules must not include this header.
struct ShaderCompileOptions
{
	std::filesystem::path SourcePath;
	std::filesystem::path IncludeDir;
	std::string EntryPoint = "main";
	ShaderStage Stage = ShaderStage::Pixel;
	ShaderTarget Target = kDefaultShaderTarget;

	bool EnableDebugInfo = false;
	bool EnableOptimizations = true;
	bool TreatWarningsAsErrors = true;
	bool StripReflection = true;
	bool StripDebugInfo = true;
	bool CaptureDebugArtifacts = false;

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
