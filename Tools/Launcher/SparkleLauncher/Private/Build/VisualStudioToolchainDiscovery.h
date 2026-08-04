#pragma once

#include <filesystem>
#include <string>

namespace SparkleLauncher
{
	struct VisualStudioToolchainDiscovery final
	{
		std::filesystem::path DiscoveryPath;
		std::filesystem::path InstallationPath;
		std::filesystem::path InstallerPath;
		std::filesystem::path ClangClPath;
		std::string WindowsSdkVersion;
	};

	VisualStudioToolchainDiscovery DiscoverVisualStudioToolchain();
	std::string ResolveVisualStudioGenerator();
}
