#pragma once

#include <filesystem>
#include <string>

namespace SparkleLauncher
{
	struct ShaderCompilerSdkStatus final
	{
		bool Available = false;
		std::filesystem::path Root;
		std::string Detail;
	};

	ShaderCompilerSdkStatus DetectShaderCompilerSdk();
}
