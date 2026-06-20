#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <string>
#include <string_view>

namespace SparkleLauncher
{
	enum class CMakeGeneratorFamily
	{
		VisualStudio,
		Ninja,
		Other
	};

	CMakeGeneratorFamily GetCMakeGeneratorFamily(std::string_view generator);
	bool CMakeGeneratorUsesPlatformArgument(std::string_view generator);
	bool CMakeGeneratorUsesToolsetArgument(std::string_view generator);
	bool CMakeGeneratorUsesNinjaMakeProgram(std::string_view generator);
	bool CMakeGeneratorUsesMsBuildArguments(std::string_view generator);
	bool CMakeGeneratorProducesSolution(std::string_view generator);
	std::string GetCMakeCachePlatformValue(const BuildToolchainStatus& toolchain);
	std::string GetCMakeCacheToolsetValue(const BuildToolchainStatus& toolchain);
}
