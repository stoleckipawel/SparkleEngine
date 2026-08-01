#pragma once

#include <filesystem>
#include <string>

namespace SparkleLauncher
{
	struct VisualStudioToolchainDiscovery final
	{
		std::filesystem::path DiscoveryPath;
		std::string WindowsSdkVersion;
	};

	VisualStudioToolchainDiscovery DiscoverVisualStudioToolchain();
	std::string ResolveVisualStudioGenerator();
}
