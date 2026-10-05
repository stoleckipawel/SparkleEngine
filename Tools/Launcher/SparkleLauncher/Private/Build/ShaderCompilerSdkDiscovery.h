#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	struct ShaderCompilerSdkStatus final
	{
		bool Available = false;
		std::filesystem::path Root;
		std::string Detail;
	};

	std::span<const std::string_view> GetShaderCompilerRequiredSdkFiles() noexcept;
	bool HasShaderCompilerStandardModules(const std::filesystem::path& binaryDirectory);
	ShaderCompilerSdkStatus DetectShaderCompilerSdk();
}
