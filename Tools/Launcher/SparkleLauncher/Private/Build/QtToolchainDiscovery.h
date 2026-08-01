#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct QtToolchainDiscovery final
	{
		bool FoundMsvcKit = false;
		std::filesystem::path QtRootPath;
		std::filesystem::path QtQmakePath;
		std::vector<std::filesystem::path> MsvcCandidates;
		std::vector<std::filesystem::path> MingwCandidates;
	};

	QtToolchainDiscovery DiscoverQtToolchain();
	std::string BuildQtToolchainStatusDetail(const QtToolchainDiscovery& discovery);
}
