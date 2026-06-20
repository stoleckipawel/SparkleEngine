#include "CMakeGeneratorModel.h"

namespace SparkleLauncher
{
	CMakeGeneratorFamily GetCMakeGeneratorFamily(std::string_view generator)
	{
		if (generator.find("Visual Studio") != std::string_view::npos)
		{
			return CMakeGeneratorFamily::VisualStudio;
		}
		if (generator.find("Ninja") != std::string_view::npos)
		{
			return CMakeGeneratorFamily::Ninja;
		}
		return CMakeGeneratorFamily::Other;
	}

	bool CMakeGeneratorUsesPlatformArgument(std::string_view generator)
	{
		return GetCMakeGeneratorFamily(generator) == CMakeGeneratorFamily::VisualStudio;
	}

	bool CMakeGeneratorUsesToolsetArgument(std::string_view generator)
	{
		return GetCMakeGeneratorFamily(generator) == CMakeGeneratorFamily::VisualStudio;
	}

	bool CMakeGeneratorUsesNinjaMakeProgram(std::string_view generator)
	{
		return GetCMakeGeneratorFamily(generator) == CMakeGeneratorFamily::Ninja;
	}

	bool CMakeGeneratorUsesMsBuildArguments(std::string_view generator)
	{
		return GetCMakeGeneratorFamily(generator) == CMakeGeneratorFamily::VisualStudio;
	}

	bool CMakeGeneratorProducesSolution(std::string_view generator)
	{
		return GetCMakeGeneratorFamily(generator) == CMakeGeneratorFamily::VisualStudio;
	}

	std::string GetCMakeCachePlatformValue(const BuildToolchainStatus& toolchain)
	{
		return CMakeGeneratorUsesPlatformArgument(toolchain.Generator) ? toolchain.Platform : std::string();
	}

	std::string GetCMakeCacheToolsetValue(const BuildToolchainStatus& toolchain)
	{
		return CMakeGeneratorUsesToolsetArgument(toolchain.Generator) ? toolchain.Toolset : std::string();
	}
}
