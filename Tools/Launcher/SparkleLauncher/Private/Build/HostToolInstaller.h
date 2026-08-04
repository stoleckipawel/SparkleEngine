#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	using HostToolInstallAvailability = bool (*)(const BuildToolchainStatus& toolchain);
	using HostToolInstallRequestFactory = std::optional<ProcessRequest> (*)(
	    const BuildToolchainStatus& toolchain,
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string& errorMessage);

	struct HostToolInstallerDefinition final
	{
		std::string Id;
		std::string DisplayName;
		std::string InstallEffect;
		HostToolInstallAvailability IsAvailable = nullptr;
		HostToolInstallRequestFactory BuildRequest = nullptr;
	};

	const std::vector<HostToolInstallerDefinition>& GetHostToolInstallerDefinitions();
	const HostToolInstallerDefinition* FindHostToolInstaller(std::string_view toolId);
	bool CanInstallHostTool(std::string_view toolId, const BuildToolchainStatus& toolchain);
	std::optional<ProcessRequest> BuildHostToolInstallRequest(
	    std::string_view toolId,
	    const BuildToolchainStatus& toolchain,
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string& errorMessage);
}
