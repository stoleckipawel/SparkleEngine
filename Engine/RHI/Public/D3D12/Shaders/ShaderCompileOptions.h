#pragma once

#include "RHIConfig.h"
#include <filesystem>
#include <vector>
#include <string>

enum class ShaderStage : uint8_t
{
	Vertex,
	Pixel,
	Geometry,
	Hull,
	Domain,
	Compute,
	Count
};

inline const char* GetShaderStagePrefix(ShaderStage stage)
{
	static constexpr const char* kPrefixes[] = {"vs", "ps", "gs", "hs", "ds", "cs"};
	static_assert(std::size(kPrefixes) == static_cast<size_t>(ShaderStage::Count));
	return kPrefixes[static_cast<size_t>(stage)];
}

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
		profile += std::to_string(RHISettings::ShaderModelMajor);
		profile += '_';
		profile += std::to_string(RHISettings::ShaderModelMinor);
		return profile;
	}
};
